#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <ArduinoOTA.h> // Incluir la biblioteca OTA
#include <FirebaseESP32.h>

// Declaraciones de funciones
void configuraWiFi();
void muestraDebugGPS();
void enviaDatosGPS();
String generaQueryString(const tmElements_t &tm, const time_t &local, float batteryLevel);
void verificaYReconectaWiFi();
void verificaConexionFirebase();
void setupOTA();
float leerNivelBateria();

// Configuración WiFi
const char* ssids[] = {"Redmi Note 12", "Sunrise", "Gatitos 2.4"};
const char* passwords[] = {"qpwo1029", "Sunrise2024", "18412044"};
int numRedes = 3; // Número de redes disponibles

// Instancias y variables globales
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
String deviceID;
bool wifiConnected = false;
double totalDistance = 0;

// Configuración de zona horaria
TimeChangeRule myDST = {"CLST", Last, Sun, Oct, 3, -180};
TimeChangeRule mySTD = {"CLT", Last, Sun, Mar, 3, -240};
Timezone myTZ(myDST, mySTD);

const unsigned long samplingInterval = 60000; // 1 minuto entre actualizaciones
unsigned long lastSampleTime = 0;

float h_nav = 0; // Horas de navegación en horas
unsigned long lastNavUpdateTime = 0; // Para marcar la última actualización cuando la lancha navega

// Define las credenciales de Firebase
#define FIREBASE_HOST "https://seatrack-909bd-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "tM8an1PKVK2jSnvci9uhasT4RcR5OzQCJXOo6KI2"

FirebaseData firebaseData;

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, 34, 12); // Usar pines 34 y 12 para GPS RX y TX respectivamente. (Lilygo)

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); // Inicia Firebase
  Firebase.reconnectWiFi(true);


  deviceID = WiFi.macAddress();
  deviceID.replace(":", "");
  configuraWiFi();
  setupOTA(); // Configura y inicia OTA
}

void loop() {
  ArduinoOTA.handle(); // Manejar cualquier potencial actualización OTA
  verificaConexionFirebase(); // Verificar y mantener la conexión con Firebase

  if (millis() - lastSampleTime >= samplingInterval) {
    lastSampleTime = millis();
    muestraDebugGPS();

    if (gps.location.isValid()) {
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
      enviaDatosGPS();
    } else {
      Serial.println("GPS: Buscando señal...");
    }
  }

  verificaYReconectaWiFi();
}

void setupOTA() {
  ArduinoOTA.setHostname("ESP32-GPS-Device");
  ArduinoOTA.onStart([]() {
    String type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void verificaConexionFirebase() {
    if (!Firebase.ready()) {
        Serial.println("Reconectando a Firebase...");
        Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    }
}

float leerNivelBateria() {
    int raw = analogRead(35); // Ajustar según el pin y el hardware específico
    float battery_voltage = raw / 4095.0 * 3.3 * 2; // Ajustar estos valores según el hardware
    return battery_voltage;
}

void enviaDatosGPS() {
  tmElements_t tm;
  tm.Year = gps.date.year() - 1970;
  tm.Month = gps.date.month();
  tm.Day = gps.date.day();
  tm.Hour = gps.time.hour();
  tm.Minute = gps.time.minute();
  tm.Second = gps.time.second();
  time_t utc = makeTime(tm);
  time_t local = myTZ.toLocal(utc);

  float batteryLevel = leerNivelBateria();
  String queryString = generaQueryString(tm, local, batteryLevel);
  
  HTTPClient http;
  String fullURL = "https://script.google.com/macros/s/AKfycbwaVPUGitFwHKSnv_h4NUZsSus1Bfjsk0sWI80vwDO6PO32qmqonAar_akHxPKiR3HU/exec" + queryString;
  http.begin(fullURL);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Datos enviados exitosamente: " + payload);
  } else {
    Serial.println("Error en la solicitud HTTP: " + http.errorToString(httpCode));
  }
  http.end();
}

String generaQueryString(const tmElements_t &tm, const time_t &local, float batteryLevel) {
  String queryString = "?id=" + deviceID +
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
                       "&h_nav=" + String(h_nav, 6) +
                       "&bat_lev=" + String(batteryLevel);
  return queryString;
}

void verificaYReconectaWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, intentando reconectar...");
    configuraWiFi();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Reconectado a WiFi");
    } else {
      Serial.println("No se pudo reconectar a ninguna red.");
    }
  }
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
      return;
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
