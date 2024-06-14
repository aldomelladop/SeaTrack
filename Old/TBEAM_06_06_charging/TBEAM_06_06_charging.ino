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
const unsigned long GPS_UPDATE_INTERVAL = 300000;  // 5 minutos
const unsigned long DATA_SEND_INTERVAL = 600000;   // 10 minutos
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
String deviceID = "SNST-TBEAM-2705";
bool wifiConnected = false;
const unsigned long samplingInterval = 60000;  // 1 minuto
unsigned long lastSampleTime = 0;
float h_nav = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
bool gpsFix = false;

float filteredLat = 0.0;
float filteredLng = 0.0;

AXP20X_Class axp;

bool usbConnected = false;
float batteryVoltage = 0.0;
bool blynkConnected = false;
unsigned long startMillis;

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  120         /* Time ESP32 will go to sleep (in seconds) */
#define TIME_TO_SLEEP_USB  255     /* Time ESP32 will go to sleep (in seconds) if USB is not connected */
#define AXP202_SLAVE_ADDRESS 0x35  /* AXP202 I2C address */

// Estructura para almacenar los datos de los centros
struct Centro {
    String id;
    float lat;
    float lng;
    float radius;  // Radio en metros
};

// Lista de centros (ejemplo)
Centro centros[] = {
    {"Centro1", -45.575, -72.068, 100}, // lat, lon, radius (en metros)
    {"Centro2", -45.578, -72.070, 100},
    // Agrega más centros según sea necesario
};

// Función para configurar el AXP202 para permitir la carga y configurar el LED
void configurarAXP202() {
    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);
    axp.setPowerOutPut(AXP202_LDO3, AXP202_ON);
    axp.setPowerOutPut(AXP202_DCDC2, AXP202_ON);
    axp.setPowerOutPut(AXP202_DCDC3, AXP202_ON);
    axp.enableChargeing(true);
    axp.setChgLEDMode(AXP20X_LED_BLINK_1HZ); // Configura el LED de carga
}

// Prototipos de funciones
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
void reportarEvento(String centroID, float lat, float lng, float speed);
void configurarBlynk();
void leerEstadoBateria();

void setup() {
    Serial.begin(115200);
    GPSSerial.begin(9600, SERIAL_8N1, 34, 12);  // Configura el puerto serie para el GPS
    
    // Inicializar AXP202
    Wire.begin(21, 22);
    if (!axp.begin(Wire, AXP202_SLAVE_ADDRESS)) {
        Serial.println("AXP202 Initialized successfully");
        configurarAXP202();
    } else {
        Serial.println("AXP202 Begin PASS");
    }

    // Configura WiFi
    configuraWiFi();
    // Configura OTA
    setupOTA();
    // Configura Blynk según la MAC
    configurarBlynk();

    // Sincroniza tiempo
    sincronizaTiempo();

    // Inicializa variables
    usbConnected = axp.isVBUSPlug();
    batteryVoltage = axp.getBattVoltage() / 1000.0;  // Convertir de mV a V
    startMillis = millis();
}

void loop() {
    ArduinoOTA.handle();
    Blynk.run();

    leerEstadoBateria();

    unsigned long currentMillis = millis();
    if (currentMillis - lastSampleTime >= samplingInterval) {
        lastSampleTime = currentMillis;
        Serial.println("Tomando muestra del GPS...");
        muestraDebugGPS();
        checkGPSFix();
        if (gpsFix) {  // Solo si los datos del GPS son válidos
            String queryString = generaQueryString();
            Serial.println("Generando queryString: " + queryString);
            enviaDatosGPS(queryString);
            enviarDatosATiempo();

            // Comprobar y reportar eventos
            for (Centro& centro : centros) {
                float distance = TinyGPSPlus::distanceBetween(filteredLat, filteredLng, centro.lat, centro.lng);
                if (distance <= centro.radius) {
                    reportarEvento(centro.id, filteredLat, filteredLng, gps.speed.kmph());
                }
            }
        }
    }

    // Verificar y reconectar WiFi si es necesario
    verificaYReconectaWiFi();

    // Verificar y entrar en modo sleep
    verificarYEntrarSleep();
}

void configuraWiFi() {
    Serial.println("Configuring WiFi...");
    WiFi.mode(WIFI_STA);
    for (int i = 0; i < sizeof(ssids) / sizeof(ssids[0]); i++) {
        wifiMulti.addAP(ssids[i], passwords[i]);
    }
    while (wifiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected to WiFi: ");
        Serial.println(WiFi.SSID());
    }
}

void setupOTA() {
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_SPIFFS
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
    Serial.println("OTA ready");
}

void muestraDebugGPS() {
    while (GPSSerial.available() > 0) {
        char c = GPSSerial.read();
        gps.encode(c);
    }

    if (gps.location.isValid()) {
        Serial.print("Latitud: ");
        Serial.println(gps.location.lat(), 6);
        Serial.print("Longitud: ");
        Serial.println(gps.location.lng(), 6);
        Serial.print("Número de Satélites: ");
        Serial.println(gps.satellites.value());
        Serial.print("Altitud: ");
        Serial.println(gps.altitude.meters());
        Serial.print("Velocidad: ");
        Serial.println(gps.speed.kmph());
        gpsFix = true;
    } else {
        Serial.println("No se pudo obtener una posición válida del GPS.");
        gpsFix = false;
    }
}

void checkGPSFix() {
    if (gps.location.isValid()) {
        gpsFix = true;
        filteredLat = gps.location.lat();
        filteredLng = gps.location.lng();
    } else {
        gpsFix = false;
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

bool enviaDatosGPS(const String& queryString) {
    HTTPClient http;
    String fullURL = SERVER_URL + queryString;
    Serial.println("Intentando conectar a: " + fullURL);
    http.begin(fullURL);
    http.setTimeout(15000);
    int httpCode = http.GET();

    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            Serial.println("Datos enviados exitosamente: " + http.getString());
            http.end();
            return true;
        } else {
            Serial.print("Error en la solicitud HTTP: ");
            Serial.println(httpCode);
            Serial.println("Respuesta: " + http.getString());
            http.end();
            return false;
        }
    } else {
        Serial.print("Error en la conexión HTTP: ");
        Serial.println(http.errorToString(httpCode));
        http.end();
        return false;
    }
}

void verificaYReconectaWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Reconnecting...");
        WiFi.disconnect();
        WiFi.begin(ssids[0], passwords[0]);
        int i = 0;
        while (WiFi.status() != WL_CONNECTED && i < 20) {
            delay(500);
            Serial.print(".");
            i++;
        }
        if(WiFi.status() == WL_CONNECTED) {
            Serial.println("Reconnected to WiFi.");
        } else {
            Serial.println("Failed to reconnect to WiFi.");
        }
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
    if (!blynkConnected) return;

    String queryString = generaQueryString();
    Blynk.virtualWrite(V0, filteredLat);
    Blynk.virtualWrite(V1, filteredLng);
    Blynk.virtualWrite(V2, gps.satellites.value());
    Blynk.virtualWrite(V3, gps.altitude.meters());
    Blynk.virtualWrite(V4, gps.speed.kmph());
}

void obtenerTiempo(struct tm &timeinfo) {
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
    }
}

void verificarYEntrarSleep() {
    // No entrar en deep sleep si la sincronización del tiempo falla o si no se pueden enviar los datos
    String queryString = generaQueryString();
    if (!enviaDatosGPS(queryString)) {
        Serial.println("Failed to send GPS data, not entering deep sleep.");
        return;
    }

    delay(15000);

    if (usbConnected) {
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    } else {
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_USB * uS_TO_S_FACTOR);
    }
    
    Serial.println("Entering deep sleep...");
    esp_deep_sleep_start();
}

void reportarEvento(String centroID, float lat, float lng, float speed) {
    struct tm timeinfo;
    obtenerTiempo(timeinfo);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d-%m-%Y", &timeinfo);

    String report = "{\"idDevice\":\"" + deviceID + "\",\"idCentro\":\"" + centroID + "\",\"Fecha\":\"" + String(timestamp) +
                    "\",\"Lat\":\"" + String(lat, 6) + "\",\"Long\":\"" + String(lng, 6) + "\",\"Speed\":\"" + String(speed) + "\"}";

    Serial.println("Reportando evento: " + report);

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(PORT_URL);
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(report);
        
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println(httpResponseCode);
            Serial.println(response);
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    }
}

void configurarBlynk() {
    String macAddress = WiFi.macAddress();
    const char* token = TOKEN_DEFAULT;

    if (macAddress.equalsIgnoreCase(MAC_SUNRISE)) {
        token = TOKEN_SUNRISE;
        deviceID = "SUNRISE";
    } else if (macAddress.equalsIgnoreCase(MAC_TRINIDAD)) {
        token = TOKEN_TRINIDAD;
        deviceID = "TRINIDAD";
    } else if (macAddress.equalsIgnoreCase(MAC_SUNSET)) {
        token = TOKEN_SUNSET;
        deviceID = "SUNSET";
    } else if (macAddress.equalsIgnoreCase(MAC_SUNSET_ICE)) {
        token = TOKEN_SUNSET_ICE;
        deviceID = "SUNSET ICE";
    }

    Serial.print("Configuring Blynk with device: ");
    Serial.print(deviceID);
    Serial.print(", MAC: ");
    Serial.println(macAddress);
    
    Blynk.config(token);
    Blynk.connect();
}

void leerEstadoBateria() {
    usbConnected = axp.isVBUSPlug();
    batteryVoltage = axp.getBattVoltage() / 1000.0;  // Convertir de mV a V

    Serial.print("USB Connected: ");
    Serial.println(usbConnected ? "Yes" : "No");
    Serial.print("Battery Voltage: ");
    Serial.print(batteryVoltage);
    Serial.println(" V");

    Serial.print("isCharging: ");
    Serial.println(axp.isChargeing() ? "YES" : "NO");
    Serial.print("isVBUSPlug: ");
    Serial.println(axp.isVBUSPlug() ? "YES" : "NO");
    Serial.print("getVbusVoltage: ");
    Serial.print(axp.getVbusVoltage());
    Serial.println("mV");
    Serial.print("getBattVoltage: ");
    Serial.print(axp.getBattVoltage());
    Serial.println("mV");

    if (axp.isBatteryConnect()) {
        Serial.print("getBatteryPercent: ");
        Serial.print(axp.getBattPercentage());
        Serial.println("%");
    }
    Serial.println();

    if (blynkConnected) {
        Blynk.virtualWrite(V13, usbConnected);
        Blynk.virtualWrite(V12, batteryVoltage);
    }
}
