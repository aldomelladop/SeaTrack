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
#include <SimpleKalmanFilter.h>

SimpleKalmanFilter kalmanLat(1, 1, 0.01);
SimpleKalmanFilter kalmanLng(1, 1, 0.01);

WiFiMulti wifiMulti;
ESP32Time rtc;
const unsigned long GPS_UPDATE_INTERVAL = 300000;
const unsigned long DATA_SEND_INTERVAL = 600000;
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
String deviceID = "TBEAM-AM-1605-1201";
bool wifiConnected = false;
const unsigned long samplingInterval = 60000;
unsigned long lastSampleTime = 0;
float h_nav = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
bool gpsFix = false;

const int maxDataPoints = 4;
int dataCount = 0;
String dataBuffer[maxDataPoints];
bool bufferEnabled = false;

float filteredLat = 0.0;
float filteredLng = 0.0;

void configuraWiFi();
void setupOTA();
void muestraDebugGPS();
void checkGPSFix();
void updateKalmanFilters();
String generaQueryString();
void manageDataSending(const String& queryString);
void enviaDatosGPS(const String& queryString);
void verificaYReconectaWiFi();
void verificaIntervalosYReinicia();
void almacenarEnBuffer(const String& queryString);
void enviarDatosDelBuffer();
void sincronizaTiempo();

void setup() {
    Serial.begin(115200);
    GPSSerial.begin(9600, SERIAL_8N1, 34, 12);
    configuraWiFi();  // Ensure WiFi is connected before continuing
    setupOTA();
    sincronizaTiempo();  // Synchronize time with NTP server
}

void loop() {
    ArduinoOTA.handle();
    unsigned long currentMillis = millis();
    if (currentMillis - lastSampleTime >= samplingInterval) {
        lastSampleTime = currentMillis;
        Serial.println("Tomando muestra del GPS...");
        muestraDebugGPS();
        checkGPSFix();
        if (gps.location.isValid() && gpsFix) {
            updateKalmanFilters();
            String queryString = generaQueryString();
            manageDataSending(queryString);
        } else {
            Serial.println("GPS: Buscando señal...");
        }
    }
    verificaYReconectaWiFi();
    verificaIntervalosYReinicia();
}

void muestraDebugGPS() {
    while (GPSSerial.available() > 0) {
        char c = GPSSerial.read();
        if (gps.encode(c)) {
            float measurementError = gps.hdop.value() * 0.5;  // Adjust measurement error based on HDOP value
            kalmanLat.setProcessNoise(measurementError);
            kalmanLng.setProcessNoise(measurementError);

            filteredLat = kalmanLat.updateEstimate(gps.location.lat());
            filteredLng = kalmanLng.updateEstimate(gps.location.lng());

            Serial.print("GPS Filtrado: Lat = ");
            Serial.print(filteredLat, 6);
            Serial.print(", Lng = ");
            Serial.println(filteredLng, 6);
        }
    }
}

void updateKalmanFilters() {
    if (!isnan(gps.location.lat()) && !isnan(gps.location.lng())) {
        float measurementError = gps.hdop.value() * 0.1;
        kalmanLat.setMeasurementError(measurementError);
        kalmanLng.setMeasurementError(measurementError);

        filteredLat = kalmanLat.updateEstimate(gps.location.lat());
        filteredLng = kalmanLng.updateEstimate(gps.location.lng());
    } else {
        Serial.println("Datos de GPS no disponibles o inválidos.");
    }
}

String generaQueryString() {
    time_t local = rtc.getEpoch();
    struct tm timeinfo;
    localtime_r(&local, &timeinfo);

    char timeStr[25];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);

    return String("?id=") + deviceID +
           "&lat=" + String(filteredLat, 6) +
           "&lng=" + String(filteredLng, 6) +
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
}

void manageDataSending(const String& queryString) {
    if (bufferEnabled) {
        almacenarEnBuffer(queryString);
    } else {
        enviaDatosGPS(queryString);
    }
}

void enviaDatosGPS(const String& queryString) {
    HTTPClient http;
    String fullURL = SERVER_URL + queryString;
    http.begin(fullURL);
    http.setTimeout(15000);
    int httpCode = http.GET();

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            Serial.println("Datos enviados exitosamente: " + http.getString());
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
}

void setupOTA() {
    ArduinoOTA.setHostname(HOSTNAME);  // Set the hostname, defined in config.h

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
    if ((currentTime - lastGPSUpdateTime > GPS_UPDATE_INTERVAL) || (currentTime - lastDataSendTime > DATA_SEND_INTERVAL)) {
        Serial.println("Reinicio necesario: intervalos máximos excedidos sin actualización del GPS o envío de datos.");
        ESP.restart();  // Restart the microcontroller
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

void almacenarEnBuffer(const String& queryString) {
    if (dataCount < maxDataPoints) {
        dataBuffer[dataCount++] = queryString;
    } else {
        Serial.println("Buffer lleno, enviando datos al servidor...");
        enviarDatosDelBuffer();
    }
}

void enviarDatosDelBuffer() {
    if (dataCount == 0) {
        Serial.println("No hay datos para enviar");
        return;
    }

    for (int i = 0; i < dataCount; i++) {
        enviaDatosGPS(dataBuffer[i]);
    }

    dataCount = 0;  // Reset the buffer counter
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
    Serial.println("Tiempo sincronizado con NTP:");
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}
