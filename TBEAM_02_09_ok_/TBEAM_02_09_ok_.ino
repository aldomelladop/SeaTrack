

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"
#include "LoRaBoards.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <TimeLib.h>
#include <ArduinoOTA.h>
#include <ESP32Time.h>
#include <BlynkSimpleEsp32.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <pgmspace.h>
#include <Preferences.h> // Biblioteca para almacenamiento en memoria interna
#include <esp_sleep.h>   // Biblioteca para manejar el deep sleep


WiFiMulti wifiMulti;
ESP32Time rtc;
Preferences preferences; // Instancia para manejar la memoria interna

const unsigned long GPS_UPDATE_INTERVAL = 300000;  // 5 minutos
const unsigned long DATA_SEND_INTERVAL = 600000;   // 10 minutos
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

// Nueva variable para la versión del firmware
const String firmwareVersion = "1.7.02.09.16:35 - Correccion-verificaYreconectaWiFi";  // Cambia esta versión según corresponda

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

bool wifiConnected = false;
const unsigned long samplingInterval = 60000;  // 1 minuto
unsigned long lastSampleTime = 0;
float h_nav = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
bool gpsFix = false;

float filteredLat = 0.0;
float filteredLng = 0.0;

WiFiClientSecure net;
PubSubClient client(net);

bool mostrarTiempoEsperaActivo = true;
bool usbEvaluator = false;
bool delayFor3min = true;
bool delayFor5min = false;
bool logs_on = false;  // Variable para controlar los logs

String lastSentAWSData = "N/A";

BlynkTimer timer;
WidgetTerminal terminal(V19);

void configuraWiFi();
void setupOTA();
void muestraDebugGPS();
void checkGPSFix();
String generaJSONPayload();
bool enviaDatosGPS();
void verificaIntervalosYReinicia();
void sincronizaTiempo();
void enviarDatosATiempo();
void obtenerTiempo(struct tm& timeinfo);
void enviarDatosPuertos();
void muestraEstadoBateria();
void mostrarTiempoEspera(unsigned long intervalo, unsigned long ultimoEnvio);
void esperar(unsigned long tiempoEspera);
void enviarReporteAWS(bool exito, const String& mensaje);
void enviarEstadoDispositivo();
void almacenarPayloadLocalmente(const String& payload);
void enviarPayloadsAlmacenados();
void restartWiFi();
void toggleLogs(bool state);
void mostrarQueueStatus();
void syncTime();
void forceSendPayloads();
void startDeepSleep();
bool reconectarAWS();
void leerComandoSerial();  // Nueva función para leer comandos desde la terminal del Arduino IDE
void processCommand(const String& command);  // Nueva función para procesar los comandos
void mostrarAyuda();  // Nueva función para mostrar la ayuda
void verificaYreconectaWiFi();
void mostrarEstadoGeneral();
void verificaYReconectaBlynk();

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

BLYNK_WRITE(V19) {
  String command = param.asStr();
  processCommand(command);  // Utilizamos la misma función para procesar comandos de Blynk y de la terminal
}

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Wire.begin();

  if (!beginPower()) {
    Serial.println("Fallo al inicializar PMU");
    while (1)
      ;
  }

  configuraWiFi();  // Nueva versión robusta que maneja reconexión
  setupOTA();
  sincronizaTiempo();
  configurarBlynk();

  net.setCACert(ROOT_CA);
  net.setCertificate(CERTIFICATE);
  net.setPrivateKey(PRIVATE_KEY);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(callback);

  if (client.connect(THING_NAME)) {
    Serial.println("Connected to AWS IOT");
  } else {
    Serial.println("AWS IoT Timeout!");
  }

  // Mostrar la versión del firmware en el terminal de Blynk
  terminal.println("Firmware version: " + firmwareVersion);
  terminal.flush();

  // Mostrar la versión del firmware en la consola de Arduino
  Serial.println("Firmware version: " + firmwareVersion);

  // Configurar el Watchdog Timer (WDT)
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 300000,  // 5 minutos
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);  // Inicializar el WDT con la configuración
  esp_task_wdt_add(NULL);  // Añadir la tarea actual al WDT

  preferences.begin("payloads", false); // Iniciar el almacenamiento en memoria interna

  lastGPSUpdateTime = millis();
  lastDataSendTime = millis();
}

void loop() {
  ArduinoOTA.handle();
  Blynk.run();
  client.loop();

  esp_task_wdt_reset();  // Reiniciar el Watchdog Timer

  if (WiFi.status() != WL_CONNECTED) {
    verificaYReconectaWiFi();
  } else {
    // Intentar enviar payloads almacenados si hay conexión WiFi
    enviarPayloadsAlmacenados();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastSampleTime >= samplingInterval) {
    lastSampleTime = currentMillis;
    Serial.println("\nTomando muestra del GPS...");
    terminal.println("\nTomando muestra del GPS...");  // Reportar a Blynk
    terminal.flush();
    muestraDebugGPS();
    esp_task_wdt_reset();  // Reiniciar el Watchdog Timer
    checkGPSFix();
    if (gps.location.isValid() && gpsFix) {
      bool envioExitoso = enviaDatosGPS();
      Blynk.virtualWrite(V16, envioExitoso ? "Sí" : "No");  // Enviar el estado del envío a Blynk

      String mensajeReporte = envioExitoso ? "Datos enviados exitosamente." : "Error: No se pudo enviar los datos.";
      enviarReporteAWS(envioExitoso, mensajeReporte);

      if (!envioExitoso) {
        almacenarPayloadLocalmente(generaJSONPayload());
      }

      enviarDatosATiempo();

      String coordenadas = String(filteredLat, 6) + ", " + String(filteredLng, 6);
      Blynk.virtualWrite(V14, coordenadas);

      struct tm timeinfo;
      obtenerTiempo(timeinfo);
      char fechaHora[20];
      strftime(fechaHora, sizeof(fechaHora), "%d-%m-%y - %H:%M", &timeinfo);
      Blynk.virtualWrite(V15, fechaHora);

      muestraEstadoBateria();
      mostrarTiempoEspera(DATA_SEND_INTERVAL, lastDataSendTime);
      Blynk.virtualWrite(V18, lastSentAWSData);  // Enviar la última fecha y hora de envío a Blynk
      esp_task_wdt_reset();  // Reiniciar el Watchdog Timer

      if (usbEvaluator) {
        delayFor3min = true;
        delayFor5min = false;
      } else {
        delayFor3min = false;
        delayFor5min = true;
      }

      if (delayFor3min) {
        esperar(180 * 1000);  // 3 min
      } else if (delayFor5min) {
        esperar(300 * 1000);  // 5 min
      }
    } else {
      Serial.println("GPS: Buscando señal...");
      terminal.println("GPS: Buscando señal...");  // Reportar búsqueda de señal GPS a Blynk
      terminal.flush();
    }
  }
  verificaIntervalosYReinicia();
}


// void verificaYReconectaWiFi() {
//     const int maxIntentosPorRed = 3;  // Máximo de intentos por cada red
//     const int maxCiclosReconexiones = 3;  // Máximo de ciclos de reconexión
//     int cicloReconexiones = 0;  // Contador de ciclos de reconexión

//     while (cicloReconexiones < maxCiclosReconexiones) {
//         Serial.printf("\nCiclo de reconexión WiFi #%d...\n", cicloReconexiones + 1);
//         terminal.printf("\nCiclo de reconexión WiFi #%d...\n", cicloReconexiones + 1);
//         terminal.flush();

//         for (int redActual = 0; redActual < sizeof(ssids) / sizeof(ssids[0]); redActual++) {
//             int intentosPorRed = 0;  // Contador de intentos por cada red

//             while (wifiMulti.run() != WL_CONNECTED && intentosPorRed < maxIntentosPorRed) {
//                 Serial.printf("Intentando conectar a %s... (Intento %d de %d)\n", ssids[redActual], intentosPorRed + 1, maxIntentosPorRed);
//                 terminal.printf("Intentando conectar a %s... (Intento %d de %d)\n", ssids[redActual], intentosPorRed + 1, maxIntentosPorRed);
//                 terminal.flush();

//                 delay(1000);  // Esperar un segundo entre intentos
//                 esp_task_wdt_reset();  // Reiniciar el Watchdog Timer
//                 intentosPorRed++;
//             }

//             if (WiFi.status() == WL_CONNECTED) {
//                 Serial.println("\nWiFi conectado.");
//                 terminal.println("\nWiFi conectado.");
//                 terminal.print("SSID: ");
//                 terminal.println(WiFi.SSID());
//                 terminal.print("IP address: ");
//                 terminal.println(WiFi.localIP());
//                 terminal.flush();

//                 // Una vez conectado al WiFi, verificar la conexión a Blynk
//                 verificaYReconectaBlynk();
//                 return;  // Salir porque ya nos conectamos a una red
//             } else {
//                 Serial.printf("No se pudo conectar a %s. Probando con la siguiente red...\n", ssids[redActual]);
//                 terminal.printf("No se pudo conectar a %s. Probando con la siguiente red...\n", ssids[redActual]);
//                 terminal.flush();
//             }
//         }

//         cicloReconexiones++;  // Incrementar el contador de ciclos de reconexión
//     }

//     // Si no se pudo conectar después de varios ciclos de reconexión
//     if (WiFi.status() != WL_CONNECTED) {
//         Serial.println("\nNo se pudo conectar a ninguna red después de varios intentos.");
//         terminal.println("\nNo se pudo conectar a ninguna red después de varios intentos.");
//         terminal.flush();
//         Serial.println("Reiniciando el dispositivo para intentar de nuevo...");
//         terminal.println("Reiniciando el dispositivo para intentar de nuevo...");
//         terminal.flush();
//         delay(100);
//         ESP.restart();  // Reiniciar el dispositivo si no se logra conectar
//     }
// }

void verificaYReconectaWiFi() {
    const int maxIntentosPorRed = 3;  // Máximo de intentos por cada red
    const int maxCiclosReconexiones = 3;  // Máximo de ciclos de reconexión
    int cicloReconexiones = 0;  // Contador de ciclos de reconexión

    while (cicloReconexiones < maxCiclosReconexiones) {
        Serial.printf("\nCiclo de reconexión WiFi #%d...\n", cicloReconexiones + 1);
        terminal.printf("\nCiclo de reconexión WiFi #%d...\n", cicloReconexiones + 1);
        terminal.flush();

        for (int redActual = 0; redActual < sizeof(ssids) / sizeof(ssids[0]); redActual++) {
            int intentosPorRed = 0;  // Contador de intentos por cada red

            while (wifiMulti.run() != WL_CONNECTED && intentosPorRed < maxIntentosPorRed) {
                Serial.printf("Intentando conectar a %s... (Intento %d de %d)\n", ssids[redActual], intentosPorRed + 1, maxIntentosPorRed);
                terminal.printf("Intentando conectar a %s... (Intento %d de %d)\n", ssids[redActual], intentosPorRed + 1, maxIntentosPorRed);
                terminal.flush();

                delay(1000);  // Esperar un segundo entre intentos
                esp_task_wdt_reset();  // Reiniciar el Watchdog Timer
                intentosPorRed++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\nWiFi conectado.");
                terminal.println("\nWiFi conectado.");
                terminal.print("SSID: ");
                terminal.println(WiFi.SSID());
                terminal.print("IP address: ");
                terminal.println(WiFi.localIP());
                terminal.flush();

                // Llama a verificaYReconectaBlynk() después de conectar WiFi
                verificaYReconectaBlynk();
                return;  // Salir porque ya nos conectamos a una red
            } else {
                Serial.printf("No se pudo conectar a %s. Probando con la siguiente red...\n", ssids[redActual]);
                terminal.printf("No se pudo conectar a %s. Probando con la siguiente red...\n", ssids[redActual]);
                terminal.flush();
            }
        }

        cicloReconexiones++;  // Incrementar el contador de ciclos de reconexión
    }

    // Si no se pudo conectar después de varios ciclos de reconexión
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nNo se pudo conectar a ninguna red después de varios intentos.");
        terminal.println("\nNo se pudo conectar a ninguna red después de varios intentos.");
        terminal.flush();
        Serial.println("Reiniciando el dispositivo para intentar de nuevo...");
        terminal.println("Reiniciando el dispositivo para intentar de nuevo...");
        terminal.flush();
        delay(100);
        ESP.restart();  // Reiniciar el dispositivo si no se logra conectar
    }
}

void verificaYReconectaBlynk() {
    if (!Blynk.connected()) {
        Serial.println("Intentando reconectar a Blynk...");
        terminal.println("Intentando reconectar a Blynk...");
        terminal.flush();

        Blynk.connect();

        if (Blynk.connected()) {
            Serial.println("Reconectado a Blynk exitosamente.");
            terminal.println("Reconectado a Blynk exitosamente.");
        } else {
            Serial.println("Error al reconectar a Blynk.");
            terminal.println("Error al reconectar a Blynk.");
        }
        terminal.flush();
    }
}

void verificaYReconectaBlynk() {
    if (!Blynk.connected()) {
        Serial.println("Blynk desconectado, intentando reconectar...");
        terminal.println("Blynk desconectado, intentando reconectar...");
        terminal.flush();

        int intentosBlynk = 0;
        const int maxIntentosBlynk = 5;  // Número máximo de intentos para reconectar a Blynk

        while (!Blynk.connected() && intentosBlynk < maxIntentosBlynk) {
            Blynk.connect();  // Intentar reconectar a Blynk
            delay(1000);  // Esperar 1 segundo antes del próximo intento

            if (Blynk.connected()) {
                Serial.println("Reconexión a Blynk exitosa.");
                terminal.println("Reconexión a Blynk exitosa.");
                terminal.flush();
                return;  // Salir porque ya nos conectamos
            } else {
                Serial.printf("Fallo al reconectar a Blynk. Intento %d de %d.\n", intentosBlynk + 1, maxIntentosBlynk);
                terminal.printf("Fallo al reconectar a Blynk. Intento %d de %d.\n", intentosBlynk + 1, maxIntentosBlynk);
                terminal.flush();
            }
            intentosBlynk++;
        }

        if (!Blynk.connected()) {
            Serial.println("No se pudo reconectar a Blynk después de varios intentos.");
            terminal.println("No se pudo reconectar a Blynk después de varios intentos.");
            terminal.flush();
        }
    }
}



void mostrarEstadoWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    terminal.println("WiFi: Conectado");
    terminal.print("SSID: ");
    terminal.println(WiFi.SSID());
    terminal.print("IP: ");
    terminal.println(WiFi.localIP());
  } else {
    terminal.println("WiFi: Desconectado");
  }
  if (logs_on) {
    terminal.flush();
  }
}

void actualizarEstadoBateria() {
  if (PMU->isBatteryConnect()) {
    terminal.print("Batería: ");
    terminal.print(PMU->getBatteryPercent());
    terminal.println("%");
    terminal.print("Voltaje: ");
    terminal.print(PMU->getBattVoltage());
    terminal.println("mV");
  } else {
    terminal.println("Batería no conectada.");
  }
  if (logs_on) {
    terminal.flush();
  }
}

void actualizarEstadoGPS() {
  terminal.print("Satélites: ");
  terminal.println(gps.satellites.value());
  terminal.print("HDOP: ");
  terminal.println(gps.hdop.hdop());
  terminal.print("Latitud: ");
  terminal.println(filteredLat, 6);
  terminal.print("Longitud: ");
  terminal.println(filteredLng, 6);

  if (logs_on) {
    terminal.flush();
  }
}

void mostrarUltimoEnvioAWS() {
  terminal.println("Último intento de envío a AWS:");
  terminal.print("Fecha y hora: ");
  terminal.println(lastSentAWSData);
  
  if (logs_on) {
    terminal.flush();
  }
}


void mostrarEstadoGeneral() {
  terminal.println("---- Estado General del Dispositivo ----");

  mostrarEstadoWiFi();          // Mostrar estado del WiFi
  actualizarEstadoBateria();    // Actualizar y mostrar el estado de la batería
  actualizarEstadoGPS();        // Actualizar y mostrar el estado del GPS
  mostrarUltimoEnvioAWS();      // Mostrar la información del último envío a AWS

  terminal.println("---------------------------------------");

  // Si los logs están activados, se asegura de que el contenido se muestre en Blynk
  if (logs_on) {
    terminal.flush();
  }
}

void configuraWiFi() {
  Serial.println("Configurando WiFi...");
  
  // Añadir todos los puntos de acceso al gestor de múltiples redes
  for (int i = 0; i < sizeof(ssids) / sizeof(ssids[0]); i++) {
    wifiMulti.addAP(ssids[i], passwords[i]);
  }

  // Intentar conectar de forma no bloqueante
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    terminal.print(".");  // Reportar intento de conexión en Blynk
    terminal.flush();
    esp_task_wdt_reset();  // Reiniciar el Watchdog Timer
  }

  Serial.println("\nWiFi conectado.");
  terminal.println("\nWiFi conectado.");  // Reportar conexión exitosa a Blynk
  terminal.print("SSID: ");
  terminal.println(WiFi.SSID());
  terminal.print("IP address: ");
  terminal.println(WiFi.localIP());
  terminal.flush();
}



void leerComandoSerial() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');  // Leer el comando hasta nueva línea
    processCommand(command);
  }
}

void processCommand(const String& command) {
  // Mostrar siempre en la terminal serial
  Serial.println("Comando recibido: " + command);
  
  // Mostrar en la terminal de Blynk solo si logs_on es true
  if (logs_on) {
    terminal.println("Comando recibido: " + command);
    terminal.flush();
  }

  if (command == "reboot") {
    Serial.println("Reiniciando el dispositivo...");
    if (logs_on) {
      terminal.println("Reiniciando el dispositivo...");
      terminal.flush();
    }
    delay(100);
    ESP.restart();
  } else if (command == "status") {
    enviarEstadoDispositivo();
  } else if (command == "restart_wifi") {
    Serial.println("Reiniciando WiFi...");
    if (logs_on) {
      terminal.println("Reiniciando WiFi...");
      terminal.flush();
    }
    restartWiFi();
    Serial.println("WiFi reiniciado.");
    if (logs_on) {
      terminal.println("WiFi reiniciado.");
      terminal.flush();
    }
  } else if (command == "log on") {
    toggleLogs(true);  // Activa los logs
  } else if (command == "log off") {
    toggleLogs(false);  // Desactiva los logs
  } else if (command == "queue_status") {
    mostrarQueueStatus();
  } else if (command == "sync_time") {
    Serial.println("Sincronizando el tiempo...");
    if (logs_on) {
      terminal.println("Sincronizando el tiempo...");
      terminal.flush();
    }
    syncTime();
  } else if (command == "force_send") {
    Serial.println("Forzando el envío de todos los payloads almacenados...");
    if (logs_on) {
      terminal.println("Forzando el envío de todos los payloads almacenados...");
      terminal.flush();
    }
    forceSendPayloads();
  } else if (command == "memory_status") {
    mostrarQueueStatus();
  } else if (command == "gen_status") {
    Serial.println("Mostrando el estado general del dispositivo...");
    if (logs_on) {
      terminal.println("Mostrando el estado general del dispositivo...");
      terminal.flush();
    }
    mostrarEstadoGeneral();
  } else if (command == "deep-sleep") {
    Serial.println("Entrando en modo Deep Sleep...");
    if (logs_on) {
      terminal.println("Entrando en modo Deep Sleep...");
      terminal.flush();
    }
    startDeepSleep();
  } else if (command == "clear") {
    Serial.println("Limpiando terminal...");
    if (logs_on) {
      terminal.clear();
      terminal.flush();
    }
  } else if (command == "help") {
    Serial.println("Comandos disponibles:");
    Serial.println("reboot       - Reinicia el dispositivo.");
    Serial.println("status       - Muestra el estado actual del dispositivo.");
    Serial.println("restart_wifi - Reinicia la conexión WiFi.");
    Serial.println("log on/off   - Activa o desactiva la visualización de logs detallados.");
    Serial.println("queue_status - Muestra el estado de la cola de payloads almacenados.");
    Serial.println("sync_time    - Sincroniza manualmente la hora con un servidor NTP.");
    Serial.println("force_send   - Fuerza el envío inmediato de todos los payloads almacenados.");
    Serial.println("memory_status- Muestra el estado de la memoria interna.");
    Serial.println("gen_status   - Muestra el estado general del dispositivo.");
    Serial.println("deep-sleep   - Pone el dispositivo en modo Deep Sleep.");
    Serial.println("clear        - Limpia la pantalla del terminal.");
    
    if (logs_on) {
      terminal.println("Comandos disponibles:");
      terminal.println("reboot       - Reinicia el dispositivo.");
      terminal.println("status       - Muestra el estado actual del dispositivo.");
      terminal.println("restart_wifi - Reinicia la conexión WiFi.");
      terminal.println("log on/off   - Activa o desactiva la visualización de logs detallados.");
      terminal.println("queue_status - Muestra el estado de la cola de payloads almacenados.");
      terminal.println("sync_time    - Sincroniza manualmente la hora con un servidor NTP.");
      terminal.println("force_send   - Fuerza el envío inmediato de todos los payloads almacenados.");
      terminal.println("memory_status- Muestra el estado de la memoria interna.");
      terminal.println("gen_status   - Muestra el estado general del dispositivo.");
      terminal.println("deep-sleep   - Pone el dispositivo en modo Deep Sleep.");
      terminal.println("clear        - Limpia la pantalla del terminal.");
      terminal.flush();
    }
  } else {
    Serial.println("Comando no reconocido.");
    if (logs_on) {
      terminal.println("Comando no reconocido.");
      terminal.flush();
    }
  }
}

void muestraDebugGPS() {
  while (GPSSerial.available() > 0) {
    char c = GPSSerial.read();
    if (gps.encode(c)) {
      filteredLat = gps.location.lat();
      filteredLng = gps.location.lng();

      Serial.printf("GPS: Lat = %f, Lng = %f\n", filteredLat, filteredLng);
      Serial.printf("Satellites: %d, HDOP: %f, Speed: %f, Course: %f\n",
                    gps.satellites.value(), gps.hdop.hdop(), gps.speed.kmph(), gps.course.deg());
    }
  }
  esp_task_wdt_reset();  // Reiniciar el Watchdog Timer
}

String generaJSONPayload() {
  StaticJsonDocument<512> doc;
  time_t local = rtc.getEpoch();
  struct tm timeinfo;
  localtime_r(&local, &timeinfo);
  doc["clientID"] = clientID;
  doc["deviceID"] = deviceID;
  doc["lat"] = filteredLat;
  doc["lng"] = filteredLng;
  doc["year"] = timeinfo.tm_year + 1900;
  doc["month"] = timeinfo.tm_mon + 1;
  doc["day"] = timeinfo.tm_mday;
  doc["hour"] = timeinfo.tm_hour;
  doc["minute"] = timeinfo.tm_min;
  doc["second"] = timeinfo.tm_sec;
  doc["speed"] = gps.speed.kmph();
  doc["sat"] = gps.satellites.value();
  doc["knots"] = gps.speed.knots();
  doc["course"] = gps.course.deg();
  doc["h_nav"] = h_nav;
  doc["hdop"] = gps.hdop.value();

  String output;
  serializeJson(doc, output);
  return output;
}

bool enviaDatosGPS() {
    String payload = generaJSONPayload();
    int qos = 1;  // Set QoS to 1 for message delivery confirmation
    bool envioExitoso = false;

    Serial.println("Iniciando el proceso de envío de datos a AWS IoT...");

    for (int i = 0; i < 3; i++) {  // Intentar enviar hasta 3 veces
        Serial.printf("Intento %d de 3: Enviando datos a AWS IoT...\n", i + 1);
        if (client.publish("sensorDevice/data", payload.c_str(), qos)) {
            Serial.println("Datos enviados exitosamente a AWS IoT.");
            lastDataSendTime = millis();
            envioExitoso = true;

            struct tm timeinfo;
            obtenerTiempo(timeinfo);
            char fechaHora[20];
            strftime(fechaHora, sizeof(fechaHora), "%d-%m-%y - %H:%M", &timeinfo);
            lastSentAWSData = String(fechaHora);

            // Log en terminal de Blynk
            terminal.println("Datos enviados a AWS IoT con éxito.");
            terminal.print("Fecha y hora: ");
            terminal.println(lastSentAWSData);
            terminal.flush();
            break;
        } else {
            Serial.println("Fallo al enviar datos a AWS IoT. Intentando de nuevo...");
            esp_task_wdt_reset();  // Reiniciar el Watchdog Timer después de un intento fallido
        }
    }

    if (!envioExitoso) {
        Serial.println("Error: No se pudo enviar los datos a AWS IoT después de 3 intentos.");
        struct tm timeinfo;
        obtenerTiempo(timeinfo);
        char fechaHora[20];
        strftime(fechaHora, sizeof(fechaHora), "%d-%m-%y - %H:%M", &timeinfo);
        lastSentAWSData = String(fechaHora) + " (Fallido)";

        // Log de error en terminal de Blynk
        terminal.println("Error: No se pudo enviar los datos a AWS IoT.");
        terminal.print("Fecha y hora: ");
        terminal.println(lastSentAWSData);
        terminal.println("Intentando reconectar a AWS IoT...");
        terminal.flush();

        // Intentar reconectar a AWS IoT
        if (reconectarAWS()) {
            Serial.println("Reconexión a AWS IoT exitosa.");
            terminal.println("Reconexión a AWS IoT exitosa. Reintentando enviar el último payload...");
            terminal.flush();
            
            // Resetear el Watchdog Timer después de reconectar
            esp_task_wdt_reset();

            // Reintentar enviar el último payload
            if (client.publish("sensorDevice/data", payload.c_str(), qos)) {
                Serial.println("Datos reenviados exitosamente a AWS IoT tras la reconexión.");
                terminal.println("Datos reenviados exitosamente a AWS IoT tras la reconexión.");
                terminal.flush();
            } else {
                Serial.println("Error: Fallo al reenviar datos tras la reconexión.");
                terminal.println("Error: Fallo al reenviar datos tras la reconexión.");
                terminal.flush();
            }
        } else {
            Serial.println("Error: Reconexión a AWS IoT fallida.");
            terminal.println("Error: Reconexión a AWS IoT fallida.");
            terminal.flush();
        }
    }

    return envioExitoso;
}

bool reconectarAWS() {
    client.disconnect();  // Desconectar la sesión actual
    delay(5000);  // Esperar un poco antes de intentar reconectar
    if (client.connect(THING_NAME)) {
        return true;
    } else {
        return false;
    }
}


void setupOTA() {
  ArduinoOTA.setHostname(HOSTNAME);

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void verificaIntervalosYReinicia() {
  unsigned long currentTime = millis();
  bool gpsUpdateTimeout = currentTime - lastGPSUpdateTime > GPS_UPDATE_INTERVAL;
  bool dataSendTimeout = currentTime - lastDataSendTime > DATA_SEND_INTERVAL;
  
  if ((gpsUpdateTimeout && !gpsFix) || (dataSendTimeout && !gpsFix)) {
    Serial.println("\nReinicio necesario: intervalos máximos excedidos sin actualización del GPS o envío de datos.");
    Serial.printf("Tiempo desde última actualización GPS: %lu ms\n", currentTime - lastGPSUpdateTime);
    Serial.printf("Tiempo desde último envío de datos: %lu ms\n", currentTime - lastDataSendTime);
    ESP.restart();
  }
}

void checkGPSFix() {
  if (gps.location.isValid()) {
    lastGPSFixTime = millis();
    gpsFix = true;
    Serial.println("GPS fix adquirido.");
  } else if (millis() - lastGPSFixTime > 300000) {
    gpsFix = false;
    Serial.println("GPS fix perdido por más de 5 minutos!");
  }
}

void sincronizaTiempo() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  setenv("TZ", "CLT4CLST,M10.2.0/00:00:00,M3.2.0/00:00:00", 1);
  tzset();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Fallo al sincronizar el tiempo con NTP");
    return;
  }
  rtc.setTimeStruct(timeinfo);

  Serial.println("Tiempo sincronizado con NTP:");
  Serial.printf("%04d-%02d-%02d %02d-%02d-%02d\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void enviarDatosATiempo() {
  struct tm timeinfo;
  obtenerTiempo(timeinfo);
  Blynk.virtualWrite(V1, timeinfo.tm_year + 1900);
  Blynk.virtualWrite(V2, timeinfo.tm_mon + 1);
  Blynk.virtualWrite(V3, timeinfo.tm_mday);
  Blynk.virtualWrite(V4, timeinfo.tm_hour);
  Blynk.virtualWrite(V5, timeinfo.tm_min);
  Blynk.virtualWrite(V6, timeinfo.tm_sec);
  Blynk.virtualWrite(V7, gps.satellites.value());
  Blynk.virtualWrite(V8, gps.speed.knots());
  Blynk.virtualWrite(V9, deviceID);
  Blynk.virtualWrite(V10, String(filteredLat, 6));
  Blynk.virtualWrite(V11, String(filteredLng, 6));
}

void obtenerTiempo(struct tm& timeinfo) {
  time_t local = rtc.getEpoch();
  localtime_r(&local, &timeinfo);
}

void muestraEstadoBateria() {
  Serial.print("\nisCharging:");
  Serial.println(PMU->isCharging() ? "YES" : "NO");
  Serial.print("isDischarge:");
  Serial.println(PMU->isDischarge() ? "YES" : "NO");
  Serial.print("isVbusIn:");
  Serial.println(PMU->isVbusIn() ? "YES" : "NO");
  Serial.print("getBattVoltage:");
  Serial.print(PMU->getBattVoltage());
  Serial.println("mV");
  Serial.print("getVbusVoltage:");
  Serial.print(PMU->getVbusVoltage());
  Serial.println("mV");
  Serial.print("getSystemVoltage:");
  Serial.print(PMU->getSystemVoltage());
  Serial.println("mV");
  if (PMU->isBatteryConnect()) {
    Serial.print("getBatteryPercent:");
    Serial.print(PMU->getBatteryPercent());
    Serial.println("%");
    Blynk.virtualWrite(V12, PMU->getBatteryPercent());
  }
  bool usbConnected = PMU->isVbusIn();
  usbEvaluator = usbConnected;
  Serial.print("USB Connected: ");
  Serial.println(usbConnected ? "YES" : "NO");
  Blynk.virtualWrite(V13, usbConnected ? "Sí" : "No");  // Enviar estado del USB a Blynk

  Serial.println();
}

void mostrarTiempoEspera(unsigned long intervalo, unsigned long ultimoEnvio) {
  if (mostrarTiempoEsperaActivo) {
    unsigned long tiempoRestante = intervalo - (millis() - ultimoEnvio);
    // Verificar que el tiempo restante no sea negativo
    if (tiempoRestante > intervalo) {
      tiempoRestante = 0;
    }
    Serial.printf("\nTiempo restante para el próximo envío de posición: %lu segundos\n", tiempoRestante / 1000);
  }
}

void setupGPS() {
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("Iniciando el GPS");
}

void esperar(unsigned long tiempoEspera) {
  unsigned long startTime = millis();
  while (millis() - startTime < tiempoEspera) {
    ArduinoOTA.handle();
    Blynk.run();
    client.loop();
    esp_task_wdt_reset();  // Reiniciar el Watchdog Timer
    delay(100);  // Pequeño delay para evitar un bucle ocupado
  }
}

void enviarReporteAWS(bool exito, const String& mensaje) {
  terminal.println("---- Reporte de Envío a AWS IoT ----");

  if (exito) {
    terminal.print("Estado: ");
    terminal.println("Exitoso");
  } else {
    terminal.print("Estado: ");
    terminal.println("Fallido");
  }

  terminal.print("Fecha y hora: ");
  terminal.println(lastSentAWSData);
  terminal.print("Mensaje: ");
  terminal.println(mensaje);
  
  terminal.println("---------------------------------------");
  terminal.flush();
}
void enviarEstadoDispositivo() {
  terminal.println("---- Estado del Dispositivo ----");

  // Estado de la conexión WiFi
  terminal.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    terminal.print("Conectado\t");
    terminal.print("SSID: ");
    terminal.print(WiFi.SSID());
    terminal.print("\tIP: ");
    terminal.println(WiFi.localIP());
    terminal.print("Nivel de señal: ");
    terminal.println(WiFi.RSSI());
  } else {
    terminal.println("Desconectado");
  }

  // Estado del GPS
  terminal.print("GPS Fix: ");
  terminal.println(gpsFix ? "Sí" : "No");
  
  // Estado de la conexión a AWS IoT
  terminal.print("AWS IoT: ");
  terminal.println(client.connected() ? "Conectado" : "Desconectado");
  terminal.print("Último envío a AWS IoT: ");
  terminal.println(lastSentAWSData);
  
  // Mostrar payloads almacenados en memoria
  int payloadCount = 0;
  for (int i = 0; i < 100; i++) {
    if (preferences.isKey(("payload" + String(i)).c_str())) {
      payloadCount++;
    }
  }
  terminal.print("Payloads almacenados: ");
  terminal.println(payloadCount);

  terminal.println("---------------------------------------");

  terminal.flush();
}


void almacenarPayloadLocalmente(const String& payload) {
  static int index = 0; // Índice para identificar cada payload
  String key = "payload" + String(index);
  preferences.putString(key.c_str(), payload); // Guardar el payload
  Serial.println("Payload almacenado localmente con clave: " + key);
  index = (index + 1) % 100; // Limitar a 100 registros para no llenar la memoria
}

void enviarPayloadsAlmacenados() {
  for (int i = 0; i < 100; i++) {
    String key = "payload" + String(i);
    if (preferences.isKey(key.c_str())) {
      String payload = preferences.getString(key.c_str());
      Serial.println("Intentando enviar payload almacenado: " + key);
      if (client.publish("sensorDevice/data", payload.c_str(), 1)) {
        Serial.println("Payload enviado exitosamente: " + key);
        preferences.remove(key.c_str()); // Eliminar payload tras el envío exitoso
      } else {
        Serial.println("Fallo al enviar payload almacenado: " + key);
        break; // Si falla el envío, no intentamos con más para preservar el orden
      }
    }
  }
}

void restartWiFi() {
  WiFi.disconnect();
  delay(1000);
  configuraWiFi();
}

void toggleLogs(bool state) {
  logs_on = state; // Cambia el estado de logs_on según el parámetro 'state'

  if (logs_on) {
    Serial.println("Logs activados.");
    terminal.println("Logs activados.");
  } else {
    Serial.println("Logs desactivados.");
    terminal.println("Logs desactivados.");
  }
  terminal.flush();
}


void mostrarQueueStatus() {
  terminal.println("---- Estado de la Cola de Payloads ----");
  
  int payloadCount = 0;  // Contador para los payloads almacenados

  for (int i = 0; i < 100; i++) {
    String key = "payload" + String(i);
    if (preferences.isKey(key.c_str())) {
      String payload = preferences.getString(key.c_str());
      terminal.print("Payload [");
      terminal.print(key);
      terminal.print("]:\t");
      terminal.println(payload);
      payloadCount++;
    }
  }

  if (payloadCount == 0) {
    terminal.println("No hay payloads almacenados.");
  } else {
    terminal.print("Total de payloads: ");
    terminal.println(payloadCount);
  }

  terminal.println("---------------------------------------");
  terminal.flush();
}

void syncTime() {
  sincronizaTiempo();
  terminal.println("Tiempo sincronizado con éxito.");
  terminal.flush();
}

void forceSendPayloads() {
  enviarPayloadsAlmacenados();
}

void startDeepSleep() {
  esp_deep_sleep_start();
}
