#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <TimeLib.h>
#include <Timezone.h>

// Declaraciones de funciones
void configuraWiFi();
void muestraDebugGPS();
void enviaDatosGPS();
String generaQueryString();
void verificaYReconectaWiFi();

// Configuración WiFi
const char* ssid = "Redmi Note 12";
const char* password = "qpwo1029";

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

const unsigned long samplingInterval = 15000; // 15 segundos entre actualizaciones
unsigned long lastSampleTime = 0;

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17);
  deviceID = WiFi.macAddress();
  deviceID.replace(":", "");
  configuraWiFi();
}

void loop() {
  muestraDebugGPS();

  if (millis() - lastSampleTime >= samplingInterval) {
    lastSampleTime = millis();

    if (gps.location.isValid()) {
      enviaDatosGPS();
    } else {
      Serial.println("GPS: Buscando señal...");
    }
  }

  verificaYReconectaWiFi();
}

// Implementación de las funciones
// Esta función maneja la conexión inicial a la red WiFi.
void configuraWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  wifiConnected = true;
  Serial.println("\nWiFi conectado");
}

// Esta función muestra información de debugging del GPS por el puerto serial.
void muestraDebugGPS() {
  while (GPSSerial.available() > 0) {
    char c = GPSSerial.read();
    if (gps.encode(c)) {
      Serial.print("GPS: ");
      Serial.print("Latitud: "); Serial.print(gps.location.lat(), 6);
      Serial.print(", Longitud: "); Serial.print(gps.location.lng(), 6);
      Serial.println();
    }
  }
}

// Esta función envía los datos del GPS a una hoja de cálculo de Google.
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

  String queryString = generaQueryString(tm, local);
  
  HTTPClient http;
  String fullURL = "https://script.google.com/macros/s/AKfycby4lY8-W-a6qcFBgNJUQaU2ouxGk4JYRj-OXXX9RdMHwPqtXC-m_PNR_3tRNeye_IQy/exec" + queryString;
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

// Esta función genera la cadena de consulta para la solicitud HTTP.
String generaQueryString(const tmElements_t &tm, const time_t &local) {
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
                       "&sat=" + gps.satellites.value();
  return queryString;
}

// Esta función verifica y, si es necesario, intenta reconectar el WiFi.
void verificaYReconectaWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, intentando reconectar...");
    WiFi.reconnect();
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.println("Reconectado a WiFi");
    }
  }
}
