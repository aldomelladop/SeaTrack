#include "config.h"
#include <WiFi.h>
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

ESP32Time rtc;
const unsigned long GPS_UPDATE_INTERVAL = 300000;
const unsigned long DATA_SEND_INTERVAL = 600000;
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

int numRedes = 3;
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
String deviceID = "TBEAM-AM-0905-1040-SNRS";
bool wifiConnected = false;
const unsigned long samplingInterval = 60000;
unsigned long lastSampleTime = 0;
float h_nav = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
bool gpsFix = false;

const int maxDataPoints = 5;
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

void setup() {
    Serial.begin(115200);
    GPSSerial.begin(9600, SERIAL_8N1, 34, 12);
    configuraWiFi();  // Ensure WiFi is connected before continuing
    setupOTA();
    // Set timezone and sync time
    configTzTime("CLT-3CLST,M10.3.0/0,M3.3.0/0", "pool.ntp.org");
}

void loop() {
    ArduinoOTA.handle();
    if (millis() - lastSampleTime >= samplingInterval) {
        lastSampleTime = millis();
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
    float measurementError = gps.hdop.value() * 0.1;
    kalmanLat.setMeasurementError(measurementError);
    kalmanLng.setMeasurementError(measurementError);
}

String generaQueryString() {
    time_t local = rtc.getEpoch();
    tmElements_t tm;
    breakTime(local, tm);
    return String("?id=") + deviceID + "&lat=" + String(filteredLat, 6) + "&lng=" + String(filteredLng, 6) + "&hdop=" + String(gps.hdop.value());
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

    if (httpCode == HTTP_CODE_OK) {
        Serial.println("Datos enviados exitosamente: " + http.getString());
    } else {
        Serial.print("Error en la solicitud HTTP: ");
        Serial.println(http.errorToString(httpCode));
    }
    http.end();
}

void configuraWiFi() {
    for (int i = 0; i < numRedes; i++) {
        Serial.print("Intentando conectar a: ");
        Serial.println(ssids[i]);
        WiFi.begin(ssids[i], passwords[i]);

        int intentos = 0;
        while (WiFi.status() != WL_CONNECTED && intentos < 10) {
            delay(500);
            Serial.print(".");
            intentos++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi conectado.");
            return;  // Exit function once connected
        } else {
            Serial.println("\nFallo en la conexión. Intentando con la siguiente red...");
        }
    }

    Serial.println("No se pudo conectar a ninguna red.");
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
        configuraWiFi();
        if (WiFi.status() == WL_CONNECTED) {
            wifiConnected = true;
            Serial.println("Reconectado a WiFi");
        } else {
            Serial.println("No se pudo reconectar a ninguna red.");
        }
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
