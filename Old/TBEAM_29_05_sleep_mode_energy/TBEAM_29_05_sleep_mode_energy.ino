#include "config.h"
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
#include <axp20x.h>

WiFiMulti wifiMulti;
ESP32Time rtc;
const unsigned long GPS_UPDATE_INTERVAL = 300000; // 5 minutes
const unsigned long DATA_SEND_INTERVAL = 600000; // 10 minutes
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
String deviceID = "";
bool wifiConnected = false;
const unsigned long samplingInterval = 60000; // 1 minute
unsigned long lastSampleTime = 0;
float h_nav = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
bool gpsFix = false;

AXP20X_Class pmu;
float batteryVoltage = 0.0;
bool usbConnected = false;
unsigned long sleepInterval = 120000; // 2 minutes by default

char auth[64];
bool blynkConnected = false;

// Declarar variables globales
int lastRSSI = 0;
const int RSSI_THRESHOLD = -80; // Umbral de RSSI en dBm
bool interferenceDetected = false;

void configuraWiFi();
void setupOTA();
void muestraDebugGPS();
void checkGPSFix();
String generaQueryString();
bool enviaDatosGPS(const String& queryString);
void verificaYReconectaWiFi();
void sincronizaTiempo();
void enviarDatosATiempo();
void obtenerTiempo(struct tm &timeinfo);
void verificarYEntrarSleep();
void checkInterference();
void setupPMU();
void actualizarEstadoBateriaYUSB();
void asignarAuthYDeviceID();

void setup() {
    Serial.begin(115200);
    GPSSerial.begin(9600, SERIAL_8N1, 34, 12);
    Wire.begin(21, 22);
    setupPMU();
    configuraWiFi();  // Ensure WiFi is connected before continuing
    asignarAuthYDeviceID();
    setupOTA();
    sincronizaTiempo();  // Synchronize time with NTP server

    if (blynkConnected) {
        Blynk.config(auth);
        Blynk.connect();  // Iniciar Blynk
    }
}

void loop() {
    if (blynkConnected) {
        ArduinoOTA.handle();
        Blynk.run();  // Ejecutar Blynk
    }

    // Obtener y transmitir datos GPS
    obtenerYTransmitirDatosGPS();

    // Actualizar estado de batería y USB
    actualizarEstadoBateriaYUSB();

    // Esperar 10 segundos antes de entrar en modo de suspensión
    delay(10000);

    // Entrar en modo de suspensión
    verificarYEntrarSleep();
}

void setupPMU() {
    if (!pmu.begin(Wire, AXP192_SLAVE_ADDRESS)) {
        Serial.println("Error al inicializar el PMU");
        while (1);
    }

    // Habilitar la carga de la batería
    pmu.setChgLEDMode(AXP20X_LED_LOW_LEVEL); // Configurar el LED de carga si está disponible

    // Configuración de los límites de carga (mA) y voltaje de la batería (mV)
    pmu.setChargeControlCur(300); // Establecer la corriente de carga en 300 mA (ajusta según tus necesidades)
    pmu.setVoffVoltage(3300); // Establecer el voltaje de apagado en 3300 mV

    // Asegurar que el ADC esté habilitado para monitorear la batería y la conexión USB
    pmu.adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, true);

    // Asegurar que las salidas de potencia estén habilitadas
    pmu.setPowerOutPut(AXP192_DCDC1, AXP202_ON); // Asegurar que el DCDC1 esté encendido
    pmu.setPowerOutPut(AXP192_DCDC2, AXP202_ON); // Asegurar que el DCDC2 esté encendido
    pmu.setPowerOutPut(AXP192_LDO2, AXP202_ON);  // Asegurar que el LDO2 esté encendido
    pmu.setPowerOutPut(AXP192_LDO3, AXP202_ON);  // Asegurar que el LDO3 esté encendido

    // Establecer el límite de corriente de carga de la batería (ajusta según tus necesidades)
    pmu.setChargeControlCur(500); // Establecer a 500 mA como ejemplo

    // Asegurar que la carga de la batería esté habilitada
    pmu.setChargeControlCur(AXP202_CCC_300MA); // Configurar la corriente de carga
    pmu.setChargingTargetVoltage(AXP202_TARGET_VOL_4_2V); // Configurar el voltaje de carga a 4.2V

    Serial.println("PMU configurado correctamente");
}

void actualizarEstadoBateriaYUSB() {
    batteryVoltage = pmu.getBattVoltage() / 1000.0;
    usbConnected = pmu.isVBUSPlug();

    // Enviar datos de batería y estado de conexión a Blynk
    if (blynkConnected) {
        Blynk.virtualWrite(V13, usbConnected);
        Blynk.virtualWrite(V12, batteryVoltage);
    }

    // Ajustar intervalo de suspensión si se detecta desconexión de USB
    if (!usbConnected) {
        sleepInterval = 135000; // 2.25 minutes if not connected to USB
    } else {
        sleepInterval = 120000; // 2 minutes if connected to USB
    }

    // Verificar interferencia
    checkInterference();
}

void obtenerYTransmitirDatosGPS() {
    bool datosTransmitidos = false;

    while (!datosTransmitidos) {
        Serial.println("Tomando muestra del GPS...");
        muestraDebugGPS();
        checkGPSFix();
        if (gps.location.isValid() && gpsFix) {
            lastNavUpdateTime = millis();
            String queryString = generaQueryString();
            datosTransmitidos = enviaDatosGPS(queryString);
            if (datosTransmitidos) {
                enviarDatosATiempo();  // Enviar datos de tiempo a Blynk

                // Enviar coordenadas como string a Blynk
                String coordenadas = String(gps.location.lat(), 6) + ", " + String(gps.location.lng(), 6);
                if (blynkConnected) {
                    Blynk.virtualWrite(V14, coordenadas);

                    // Enviar fecha y hora como string a Blynk
                    struct tm timeinfo;
                    obtenerTiempo(timeinfo);
                    char fechaHora[20];
                    strftime(fechaHora, sizeof(fechaHora), "%d-%m-%y - %H:%M", &timeinfo);
                    Blynk.virtualWrite(V15, fechaHora);
                }
            }
        } else {
            Serial.println("GPS: Buscando señal...");
        }

        // Si no se transmitieron datos, esperar un poco antes de intentar de nuevo
        if (!datosTransmitidos) {
            delay(5000);
        }
    }
}

void muestraDebugGPS() {
    while (GPSSerial.available() > 0) {
        char c = GPSSerial.read();
        if (gps.encode(c)) {
            Serial.print("GPS: Lat = ");
            Serial.print(gps.location.lat(), 6);
            Serial.print(", Lng = ");
            Serial.println(gps.location.lng(), 6);
        }
    }
    int rssi = WiFi.RSSI();
    Serial.print("RSSI: ");
    Serial.println(rssi);
    lastRSSI = rssi; // Guarda el valor del último RSSI leído

    Serial.println("Nivel de batería: " + String(batteryVoltage));
    Serial.println("USB_conectado: " + String(usbConnected));
}

String generaQueryString() {
    time_t local = rtc.getEpoch();
    struct tm timeinfo;
    localtime_r(&local, &timeinfo);

    char timeStr[25];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

    String queryString = String("?id=") + deviceID +
                         "&lat=" + String(gps.location.lat(), 6) +
                         "&lng=" + String(gps.location.lng(), 6) +
                         "&year=" + (timeinfo.tm_year + 1900) +
                         "&month=" + (timeinfo.tm_mon + 1) +
                         "&day=" + timeinfo.tm_mday +
                         "&hour=" + timeinfo.tm_hour +
                         "&minute=" + timeinfo.tm_min +
                         "&second=" + timeinfo.tm_sec +
                         "&speed=" + gps.speed.kmph() +
                         "&sat=" + gps.satellites.value() +
                         "&knots=" + gps.speed.knots() +
                         "&course=" + gps.course.deg() +
                         "&h_nav=" + h_nav +
                         "&hdop=" + gps.hdop.value();
    
    return queryString;
}

bool enviaDatosGPS(const String& queryString) {
    HTTPClient http;
    String fullURL = SERVER_URL + queryString;
    Serial.println("Intentando conectar a: " + fullURL);
    http.begin(fullURL);
    http.setTimeout(15000);
    int httpCode = http.GET();

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND) {
            Serial.println("Datos enviados exitosamente: " + http.getString());
            return true; // Datos transmitidos exitosamente
        } else {
            Serial.print("Error en la solicitud HTTP: ");
            Serial.println(httpCode);
            Serial.println("Respuesta: " + http.getString());
        }
    } else {
        Serial.print("Error en la conexión HTTP: ");
        Serial.println(http.errorToString(httpCode));
    }
    http.end();
    return false;
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
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Conectado a: ");
    Serial.println(WiFi.SSID());
}

void setupOTA() {
    if (blynkConnected) {
        ArduinoOTA.setHostname(HOSTNAME);  // Set the hostname, defined in config.h
        ArduinoOTA.setPassword(OTA_PASSWORD);  // Set the OTA password

        ArduinoOTA.onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH) {
                type = "sketch";
            } else { // U_SPIFFS
                type = "filesystem";
            }
            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            Serial.println("Start updating " + type);
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("\nEnd");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) {
                Serial.println("Auth Failed");
            } else if (error == OTA_BEGIN_ERROR) {
                Serial.println("Begin Failed");
            } else if (error == OTA_CONNECT_ERROR) {
                Serial.println("Connect Failed");
            } else if (error == OTA_RECEIVE_ERROR) {
                Serial.println("Receive Failed");
            } else if (error == OTA_END_ERROR) {
                Serial.println("End Failed");
            }
        });

        ArduinoOTA.begin();
        Serial.println("OTA Ready");
    }
}

void verificaYReconectaWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado, intentando reconectar...");
        while (wifiMulti.run() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nReconectado a WiFi.");
        Serial.print("Conectado a: ");
        Serial.println(WiFi.SSID());
    }
}

void checkGPSFix() {
    if (gps.location.isValid()) {
        lastGPSFixTime = millis();
        gpsFix = true;
        Serial.println("GPS fix adquirido.");
    } else if (millis() - lastGPSFixTime > 300000) {  // 5 minutes without GPS fix
        gpsFix = false;
        Serial.println("GPS fix perdido por más de 5 minutos!");
    }
}

void sincronizaTiempo() {
    // Configura el servidor NTP y la zona horaria
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // GMT offset y daylight offset configurados en 0
    
    // Configura la zona horaria específica para Chile continental
    // CLT: Chile Standard Time (UTC-4), CLST: Chile Summer Time (UTC-3)
    setenv("TZ", "CLT4CLST,M10.2.0/00:00:00,M3.2.0/00:00:00", 1); // Esta configuración considera la transición a horario de verano
    tzset();  // Aplica la configuración de zona horaria

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Fallo al sincronizar el tiempo con NTP");
        return;
    }
    rtc.setTimeStruct(timeinfo); // Sincroniza el RTC con el tiempo NTP

    Serial.println("Tiempo sincronizado con NTP:");
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void enviarDatosATiempo() {
    struct tm timeinfo;
    obtenerTiempo(timeinfo);
    if (blynkConnected) {
        Blynk.virtualWrite(V1, timeinfo.tm_year + 1900);
        Blynk.virtualWrite(V2, timeinfo.tm_mon + 1);
        Blynk.virtualWrite(V3, timeinfo.tm_mday);
        Blynk.virtualWrite(V4, timeinfo.tm_hour);
        Blynk.virtualWrite(V5, timeinfo.tm_min);
        Blynk.virtualWrite(V6, timeinfo.tm_sec);
        Blynk.virtualWrite(V7, gps.satellites.value());
        Blynk.virtualWrite(V8, gps.speed.knots());
        Blynk.virtualWrite(V9, deviceID);  // Asegúrate de enviar el deviceID como String
        Blynk.virtualWrite(V10, String(gps.location.lat(), 6));
        Blynk.virtualWrite(V11, String(gps.location.lng(), 6));
    }
}

void obtenerTiempo(struct tm &timeinfo) {
    time_t local = rtc.getEpoch();
    localtime_r(&local, &timeinfo);
}

void verificarYEntrarSleep() {
    Serial.println("Entrando en modo de suspensión...");
    esp_sleep_enable_timer_wakeup(sleepInterval * 1000); // Configurar el temporizador de despertar
    esp_deep_sleep_start();
}

void checkInterference() {
    int rssi = WiFi.RSSI();
    if (rssi < RSSI_THRESHOLD && lastRSSI >= RSSI_THRESHOLD) {
        interferenceDetected = true;
        Serial.println("Interferencia detectada: RSSI ha caído abruptamente");
        // Puedes enviar una alerta a través de Blynk
        if (blynkConnected) {
            Blynk.virtualWrite(V16, "Interferencia detectada");
        }
    }
    lastRSSI = rssi; // Actualiza el último valor de RSSI leído
}

void asignarAuthYDeviceID() {
    String macAddress = WiFi.macAddress();
    Serial.println("MAC Address: " + macAddress);

    struct tm timeinfo;
    obtenerTiempo(timeinfo);
    char deviceIDBuff[25];
    strftime(deviceIDBuff, sizeof(deviceIDBuff), "%d%m", &timeinfo);

    if (macAddress.equalsIgnoreCase(MAC_SUNRISE)) {
        strcpy(auth, TOKEN_SUNRISE);
        deviceID = "SNRS-TBEAM-" + String(deviceIDBuff);
        blynkConnected = true;
    } else if (macAddress.equalsIgnoreCase(MAC_TRINIDAD)) {
        strcpy(auth, TOKEN_TRINIDAD);
        deviceID = "TRND-TBEAM-" + String(deviceIDBuff);
        blynkConnected = true;
    } else if (macAddress.equalsIgnoreCase(MAC_SUNSET)) {
        strcpy(auth, TOKEN_SUNSET);
        deviceID = "SNST-TBEAM-" + String(deviceIDBuff);
        blynkConnected = true;
    } else if (macAddress.equalsIgnoreCase(MAC_SUNSET_ICE)) {
        strcpy(auth, TOKEN_SUNSET_ICE);
        deviceID = "SNST-ICE-TBEAM-" + String(deviceIDBuff);
        blynkConnected = true;
    } else {
        deviceID = macAddress;
        blynkConnected = false;
    }

    Serial.println("Device ID: " + deviceID);
    if (blynkConnected) {
        Serial.println("Blynk Auth Token: " + String(auth));
    } else {
        Serial.println("No se enviará a Blynk, pero se guardarán los datos en la hoja de cálculo.");
    }
}