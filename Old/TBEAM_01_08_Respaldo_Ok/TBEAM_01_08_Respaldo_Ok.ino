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

WiFiMulti wifiMulti;
ESP32Time rtc;
Preferences preferences; // Instancia para manejar la memoria interna

const unsigned long GPS_UPDATE_INTERVAL = 300000;  // 5 minutos
const unsigned long DATA_SEND_INTERVAL = 600000;   // 10 minutos
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

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

String lastSentAWSData = "N/A";

BlynkTimer timer;
WidgetTerminal terminal(V19);

void configuraWiFi();
void setupOTA();
void muestraDebugGPS();
void checkGPSFix();
String generaJSONPayload();
bool enviaDatosGPS();
void verificaYReconectaWiFi();
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
  if (command == "reboot") {
    terminal.println("Comando recibido: reboot. Reiniciando el dispositivo...");
    terminal.flush();
    delay(100);
    ESP.restart();
  } else if (command == "status") {
    enviarEstadoDispositivo();
  } else {
    terminal.println("Comando no reconocido.");
    terminal.flush();
  }
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

  configuraWiFi();
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
    }
  }
  verificaIntervalosYReinicia();
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

  Serial.println("Iniciando envío de datos a AWS IoT...");
  for (int i = 0; i < 3; i++) {  // Intentar enviar hasta 3 veces
    if (client.publish("sensorDevice/data", payload.c_str(), qos)) {
      Serial.println("\nDatos enviados a AWS IoT: ");
      Serial.println(payload);
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
    terminal.flush();
  }
  return envioExitoso;
}

void configuraWiFi() {
  for (int i = 0; i < sizeof(ssids) / sizeof(ssids[0]); i++) {
    wifiMulti.addAP(ssids[i], passwords[i]);
  }

  Serial.println("Conectando a WiFi...");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado.");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
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

void verificaYReconectaWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, intentando reconectar...");
    while (wifiMulti.run() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nReconectado a WiFi.");
  }
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
  if (exito) {
    terminal.println("Último envío exitoso:");
  } else {
    terminal.println("Último intento de envío fallido:");
  }

  terminal.print("Fecha y hora: ");
  terminal.println(lastSentAWSData);
  terminal.println(mensaje);
  terminal.flush();
}

void enviarEstadoDispositivo() {
  terminal.println("Estado del dispositivo:");

  // Estado de la conexión WiFi
  if (WiFi.status() == WL_CONNECTED) {
    terminal.println("WiFi: Conectado");
    terminal.print("SSID: ");
    terminal.println(WiFi.SSID());
    terminal.print("IP: ");
    terminal.println(WiFi.localIP());
  } else {
    terminal.println("WiFi: Desconectado");
  }

  // Estado del GPS
  terminal.print("GPS Fix: ");
  terminal.println(gpsFix ? "Sí" : "No");

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
  int totalPayloads = 0;
  int sentPayloads = 0;

  // Contar el total de payloads almacenados
  for (int i = 0; i < 100; i++) {
    String key = "payload" + String(i);
    if (preferences.isKey(key.c_str())) {
      totalPayloads++;
    }
  }

  for (int i = 0; i < 100; i++) {
    String key = "payload" + String(i);
    if (preferences.isKey(key.c_str())) {
      String payload = preferences.getString(key.c_str());
      Serial.println("Intentando enviar payload almacenado: " + key);
      if (client.publish("sensorDevice/data", payload.c_str(), 1)) {
        Serial.println("Payload enviado exitosamente: " + key);
        preferences.remove(key.c_str()); // Eliminar payload tras el envío exitoso
        sentPayloads++;
        int progress = (sentPayloads * 100) / totalPayloads;
        Serial.printf("Progreso de envío: %d%%\n", progress);
      } else {
        Serial.println("Fallo al enviar payload almacenado: " + key);
        break; // Si falla el envío, no intentamos con más para preservar el orden
      }
    }
  }
}
