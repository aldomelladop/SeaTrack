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
#include "LoRaBoards.h"

WiFiMulti wifiMulti;
ESP32Time rtc;
const unsigned long GPS_UPDATE_INTERVAL = 300000;
const unsigned long DATA_SEND_INTERVAL = 600000;
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
String deviceID = "SNRS-TBEAM-3005";
bool wifiConnected = false;
const unsigned long samplingInterval = 60000;
unsigned long lastSampleTime = 0;
float h_nav = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
bool gpsFix = false;

const int batteryPin = 35; 
const int usbPowerPin = 36;
float batteryVoltage = 0.0;
bool usbConnected = false;
unsigned long sleepInterval = 90000;

const int RSSI_THRESHOLD = -80; 
int lastRSSI = 0;
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

void setup() {
    Serial.begin(115200);
    GPSSerial.begin(9600, SERIAL_8N1, 34, 12);
    pinMode(batteryPin, INPUT);
    pinMode(usbPowerPin, INPUT);
    configuraWiFi();  
    setupOTA();
    sincronizaTiempo();
    Blynk.config(auth);
    Blynk.connect();
    usbConnected = digitalRead(usbPowerPin);
    if (!usbConnected) {
        sleepInterval = 105000;
    }
    setupBoards();
}

void loop() {
    ArduinoOTA.handle();
    Blynk.run();
    obtenerYTransmitirDatosGPS();
    updateBatteryStatus();
    delay(10000);
    verificarYEntrarSleep();
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
                enviarDatosATiempo();  
                String coordenadas = String(gps.location.lat(), 6) + ", " + String(gps.location.lng(), 6);
                Blynk.virtualWrite(V14, coordenadas);
                struct tm timeinfo;
                obtenerTiempo(timeinfo);
                char fechaHora[20];
                strftime(fechaHora, sizeof(fechaHora), "%d-%m-%y - %H:%M", &timeinfo);
                Blynk.virtualWrite(V15, fechaHora);
            }
        } else {
            Serial.println("GPS: Buscando señal...");
        }

        usbConnected = digitalRead(usbPowerPin);
        Blynk.virtualWrite(V13, usbConnected);

        if (!usbConnected) {
            sleepInterval = 105000;
        } else {
            sleepInterval = 90000;
        }

        checkInterference();

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
    lastRSSI = rssi;

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
            return true;
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
    for (int i = 0; i < NUM_SSIDS; i++) {
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
}

void setupOTA() {
    ArduinoOTA.setHostname(HOSTNAME);  
    ArduinoOTA.setPassword(OTA_PASSWORD);  

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {
            type = "filesystem";
        }
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
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
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
    Blynk.virtualWrite(V10, String(gps.location.lat(), 6));
    Blynk.virtualWrite(V11, String(gps.location.lng(), 6));
}

void obtenerTiempo(struct tm &timeinfo) {
    time_t local = rtc.getEpoch();
    localtime_r(&local, &timeinfo);
}

void verificarYEntrarSleep() {
    Serial.println("Entrando en modo de suspensión...");
    esp_sleep_enable_timer_wakeup(sleepInterval * 1000);
    esp_deep_sleep_start();
}

void checkInterference() {
    int rssi = WiFi.RSSI();
    if (rssi < RSSI_THRESHOLD && lastRSSI >= RSSI_THRESHOLD) {
        interferenceDetected = true;
        Serial.println("Interferencia detectada: RSSI ha caído abruptamente");
        Blynk.virtualWrite(V16, "Interferencia detectada");
    }
    lastRSSI = rssi;
}
