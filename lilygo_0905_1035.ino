#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <TimeLib.h>
#include <ArduinoOTA.h>
#include <ESP32Time.h>

// Configuración de ESP32Time para manejar la zona horaria
ESP32Time rtc;

// Definiciones de los intervalos máximos
const unsigned long GPS_UPDATE_INTERVAL = 300000; // 5 minutos
const unsigned long DATA_SEND_INTERVAL = 600000; // 10 minutos

// Variables para controlar el tiempo desde la última actualización
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

void configuraWiFi();
void muestraDebugGPS();
void enviaDatosGPS();
String generaQueryString(const tmElements_t &tm, const time_t &local);
void verificaYReconectaWiFi();
void setupOTA();
void checkGPSFix();
void verificaIntervalosYReinicia();
void almacenarEnBuffer(const String& queryString);
void enviarDatosDelBuffer();

int numRedes = 3; // Número de redes disponibles
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
String deviceID;
bool wifiConnected = false;
double totalDistance = 0;
const unsigned long samplingInterval = 60000;
unsigned long lastSampleTime = 0;
float h_nav = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
bool gpsFix = false;

const int maxDataPoints = 5; // Número máximo de datos a almacenar antes de enviarlos
int dataCount = 0;
String dataBuffer[maxDataPoints];

// Activa o desactiva el almacenamiento en buffer
bool bufferEnabled = false; // Cambiar a false para desactivar el buffer

void setup() {
    Serial.begin(115200);
    Serial.println("Configurando GPS...");
    GPSSerial.begin(9600, SERIAL_8N1, 34, 12);
    deviceID = "TBEAM-AM-0905-1040-SNRS";
    configuraWiFi();
    setupOTA();

    // Configura la zona horaria
    configTzTime("CLT-3CLST,M10.3.0/0,M3.3.0/0", "pool.ntp.org");
}

void loop() {
    ArduinoOTA.handle();

    tmElements_t tm; // Define la estructura tmElements_t
    time_t local = rtc.getEpoch(); // Obtiene la hora local

    if (millis() - lastSampleTime >= samplingInterval) {
        lastSampleTime = millis();
        Serial.println("Tomando muestra del GPS...");
        muestraDebugGPS();
        checkGPSFix();

        if (gps.location.isValid() && gpsFix) {
            lastGPSUpdateTime = millis();  // Actualiza el tiempo de la última actualización del GPS
            float knots = gps.speed.kmph() * 0.539957;
            if (knots > 0.2) {
                if (lastNavUpdateTime == 0) {
                    lastNavUpdateTime = millis();
                } else {
                    h_nav += (millis() - lastNavUpdateTime) / 3600000.0;
                    lastNavUpdateTime = millis();
                }
            } else {
                lastNavUpdateTime = 0;
            }

            String queryString = generaQueryString(tm, local);
            if (bufferEnabled) {
                Serial.println("Almacenando datos en el buffer...");
                almacenarEnBuffer(queryString);
            } else {
                Serial.println("Enviando datos al servidor...");
                enviarDatosDelBuffer();
                enviaDatosGPS(queryString);
            }
        } else {
            Serial.println("GPS: Buscando señal...");
        }
    }

    verificaYReconectaWiFi();
    verificaIntervalosYReinicia();  // Verifica los intervalos y reinicia si es necesario
}

void verificaIntervalosYReinicia() {
    unsigned long currentTime = millis();
    if ((currentTime - lastGPSUpdateTime > GPS_UPDATE_INTERVAL) || (currentTime - lastDataSendTime > DATA_SEND_INTERVAL)) {
        Serial.println("Reinicio necesario: intervalos máximos excedidos sin actualización del GPS o envío de datos.");
        ESP.restart();  // Reinicia el microcontrolador
    }
}

void checkGPSFix() {
    if (gps.location.isValid()) {
        lastGPSFixTime = millis();
        gpsFix = true;
        Serial.println("GPS fix adquirido.");
    } else if (millis() - lastGPSFixTime > 300000) { // 5 minutes without GPS fix
        gpsFix = false;
        Serial.println("GPS fix perdido por más de 5 minutos!");
    }
}

void enviaDatosGPS(const String& queryString) {
    HTTPClient http;

    // Construye la URL completa con el queryString
    String fullURL = String(SERVER_URL) + queryString;
    Serial.print("Enviando solicitud a: ");
    Serial.println(fullURL);

    // Aumenta el tiempo de espera
    http.setTimeout(15000); // 15 segundos

    // Realiza la solicitud HTTP GET al servidor URL
    http.begin(fullURL);
    int httpCode = http.GET();

    // Verifica si la solicitud fue exitosa
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Datos enviados exitosamente: " + payload);
    } else {
        Serial.print("Error en la solicitud HTTP: ");
        Serial.println(http.errorToString(httpCode));
        // Si falla, reintentar
        enviarDatosDelBuffer();
    }

    // Cierra la conexión HTTP
    http.end();
}

String generaQueryString(const tmElements_t &tm, const time_t &local) {
    return "?id=" + deviceID +
           "&lat=" + String(gps.location.lat(), 6) +
           "&lng=" + String(gps.location.lng(), 6) +
           "&year=" + String(tm.Year + 1970) +
           "&month=" + String(tm.Month) +
           "&day=" + String(tm.Day) +
           "&hour=" + String(hour(local)) +
           "&minute=" + String(minute(local)) +
           "&second=" + String(second(local)) +
           "&speed=" + String(gps.speed.kmph()) +
           "&knots=" + String(gps.speed.kmph() * 0.539957) +
           "&sat=" + gps.satellites.value() +
           "&course=" + String(gps.course.deg(), 2) +
           "&hdop=" + String(gps.hdop.value()) +
           "&h_nav=" + String(h_nav, 6);
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

// Esta función maneja la conexión inicial a la red WiFi.
void configuraWiFi() {
    for (int i = 0; i < numRedes; i++) {
        Serial.print("Intentando conectar a: ");
        Serial.println(ssids[i]);
        WiFi.begin(ssids[i], passwords[i]);

        int intentos = 0;
        while (WiFi.status() != WL_CONNECTED && intentos < 10) { // Intenta hasta 10 veces por red
            delay(500);
            Serial.print(".");
            intentos++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nWiFi conectado.");
            return; // Sale de la función si se conecta exitosamente
        } else {
            Serial.println("\nFallo en la conexión. Intentando con la siguiente red...");
        }
    }

    Serial.println("No se pudo conectar a ninguna red.");
}

void muestraDebugGPS() {
    while (GPSSerial.available() > 0) {
        char c = GPSSerial.read();
        if (gps.encode(c)) {
            Serial.print("GPS: ");
            Serial.print("Latitud: "); Serial.print(gps.location.lat(), 6);
            Serial.print(", Longitud: "); Serial.print(gps.location.lng(), 6);
            Serial.print(" Satélites conectados: "); Serial.print(gps.satellites.value());
            Serial.println();
        }
    }
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

    dataCount = 0; // Reinicia el contador del buffer
}