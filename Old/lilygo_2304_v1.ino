#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <ArduinoOTA.h>

void configuraWiFi();
void muestraDebugGPS();
void enviaDatosGPS();
String generaQueryString();
void verificaYReconectaWiFi();
void setupOTA();
void checkGPSFix();

int numRedes = 3; // Número de redes disponibles
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
String deviceID;
bool wifiConnected = false;
double totalDistance = 0;
Timezone myTZ(myDST, mySTD);
const unsigned long samplingInterval = 45000;
unsigned long lastSampleTime = 0;
float h_nav = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
bool gpsFix = false;

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, 34, 12);
  deviceID = WiFi.macAddress();
  deviceID.replace(":", "");
  configuraWiFi();
  setupOTA();
}

void loop() {
  ArduinoOTA.handle();

  if (millis() - lastSampleTime >= samplingInterval) {
    lastSampleTime = millis();
    muestraDebugGPS();
    checkGPSFix();

    if (gps.location.isValid() && gpsFix) {
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

void checkGPSFix() {
  if (gps.location.isValid()) {
    lastGPSFixTime = millis();
    gpsFix = true;
  } else if (millis() - lastGPSFixTime > 300000) { // 5 minutes without GPS fix
    gpsFix = false;
    Serial.println("GPS fix perdido por más de 5 minutos!");
    // Implement additional notifications or actions here if needed
  }
}

void enviaDatosGPS() {
  if (!gpsFix) return; // Do not proceed if GPS fix is lost

  tmElements_t tm;
  tm.Year = gps.date.year() - 1970;
  tm.Month = gps.date.month();
  tm.Day = gps.date.day();
  tm.Hour = gps.time.hour();
  tm.Minute = gps.time.minute();
  tm.Second = gps.time.second();
  time_t utc = makeTime(tm);
  time_t local = myTZ.toLocal(utc);

  HTTPClient http;
  String fullURL = String(SERVER_URL) + generaQueryString(tm, local);
  http.begin(fullURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Datos enviados exitosamente: " + payload);
  } else {
    Serial.print("Error en la solicitud HTTP: ");
    Serial.println(http.errorToString(httpCode));
  }
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

