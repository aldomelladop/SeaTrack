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
#include <BlynkSimpleEsp32.h>

// Configuración del filtro de Kalman
SimpleKalmanFilter kalmanLat(1, 1, 0.01);
SimpleKalmanFilter kalmanLng(1, 1, 0.01);

ESP32Time rtc;

const unsigned long GPS_UPDATE_INTERVAL = 300000;
const unsigned long DATA_SEND_INTERVAL = 600000;
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
String deviceID = "TBEAM-1705-1420";

const unsigned long samplingInterval = 60000;
unsigned long lastSampleTime = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
float h_nav = 0;
bool gpsFix = false;

const int maxDataPoints = 4;
int dataCount = 0;
String dataBuffer[maxDataPoints];
bool bufferEnabled = false;

float filteredLat = 0.0;
float filteredLng = 0.0;

// Variables para activar/desactivar funciones
bool kalmanFilterEnabled = false;
bool enviarDatosSpreadsheet = false;

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
bool datosGPSValidos(double lat, double lng);
void configurarBlynk(const char* ssid, const char* password);
void enviarDatosATiempo();
void obtenerTiempo(struct tm &timeinfo);

// Sección de activación/desactivación de funciones desde Blynk
BLYNK_WRITE(V12) {
    enviarDatosSpreadsheet = param.asInt();
    Serial.print("Enviar datos a GSheets: ");
    Serial.println(enviarDatosSpreadsheet);
}

BLYNK_WRITE(V13) {
    kalmanFilterEnabled = param.asInt();
    Serial.print("Filtro Kalman: ");
    Serial.println(kalmanFilterEnabled);
}

void setup() {
    Serial.begin(115200);
    GPSSerial.begin(9600, SERIAL_8N1, 34, 12);
    configuraWiFi();  // Asegurarse de que WiFi esté conectado antes de continuar
    setupOTA();
    sincronizaTiempo();  // Sincronizar tiempo con el servidor NTP
    Blynk.config(auth);
    Blynk.connect();  // Iniciar Blynk
}

void loop() {
    ArduinoOTA.handle();
    Blynk.run();  // Ejecutar Blynk
    unsigned long currentMillis = millis();
    if (currentMillis - lastSampleTime >= samplingInterval) {
        lastSampleTime = currentMillis;
        Serial.println("Tomando muestra del GPS...");
        muestraDebugGPS();
        checkGPSFix();
        if (gps.location.isValid() && gpsFix && datosGPSValidos(gps.location.lat(), gps.location.lng())) {
            updateKalmanFilters();
            String queryString = generaQueryString();
            manageDataSending(queryString);
            enviarDatosATiempo();  // Enviar datos de tiempo a Blynk

            if (enviarDatosSpreadsheet) {
                enviaDatosGPS(queryString);
            }

            // Enviar coordenadas como string a Blynk
            String coordenadas = String(filteredLat, 6) + ", " + String(filteredLng, 6);
            Blynk.virtualWrite(V14, coordenadas);

            // Enviar fecha y hora como string a Blynk
            struct tm timeinfo;
            obtenerTiempo(timeinfo);
            char fechaHora[20];
            strftime(fechaHora, sizeof(fechaHora), "%d-%m-%y - %H:%M", &timeinfo);
            Blynk.virtualWrite(V15, fechaHora);
        } else {
            Serial.println("GPS: Buscando señal...");
        }
    }
    verificaYReconectaWiFi();
}

void updateKalmanFilters() {
    if (kalmanFilterEnabled) {
        filteredLat = kalmanLat.updateEstimate(gps.location.lat());
        filteredLng = kalmanLng.updateEstimate(gps.location.lng());
    } else {
        filteredLat = gps.location.lat();
        filteredLng = gps.location.lng();
    }
}

bool datosGPSValidos(double lat, double lng) {
    // Implementar lógica para filtrar datos erráticos y valores 'nan'
    if (isnan(lat) || isnan(lng) || lat == 0.0 || lng == 0.0) {
        return false;
    }
    // Agregar más condiciones según sea necesario
    return true;
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
        if (enviarDatosSpreadsheet) {
            enviaDatosGPS(queryString);
        }
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
    int numNetworks = sizeof(ssids) / sizeof(ssids[0]);
    for (int i = 0; i < numNetworks; i++) {
        Serial.print("Conectando a ");
        Serial.println(ssids[i]);
        WiFi.begin(ssids[i], passwords[i]);

        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nConectado a WiFi.");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            configurarBlynk(ssids[i], passwords[i]);  // Configurar Blynk con las credenciales correctas
            return;
        } else {
            Serial.println("\nFalló la conexión a WiFi.");
        }
    }

    Serial.println("No se pudo conectar a ninguna red WiFi.");
}

void configurarBlynk(const char* ssid, const char* password) {
    // Reconfigura Blynk con las credenciales correctas
    Blynk.disconnect();
    Blynk.config(auth);
    Blynk.connectWiFi(ssid, password);
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
        configuraWiFi();  // Reconectar a WiFi y reconfigurar Blynk
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
    Serial.printf("%04d-%02d-%02d %02:%02:%02d\n",
                  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
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
    Blynk.virtualWrite(V9, deviceID);  // Asegúrate de enviar el deviceID como String
    Blynk.virtualWrite(V10, String(filteredLat, 6));
    Blynk.virtualWrite(V11, String(filteredLng, 6));
}

void obtenerTiempo(struct tm &timeinfo) {
    time_t local = rtc.getEpoch();
    localtime_r(&local, &timeinfo);
}
