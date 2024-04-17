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
// Lista de redes WiFi
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

const unsigned long samplingInterval = 60000; // 15 segundos entre actualizaciones
unsigned long lastSampleTime = 0;


// Variables globales
float h_nav = 0; // Horas de navegación en horas
unsigned long lastNavUpdateTime = 0; // Para marcar la última actualización cuando la lancha navega

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17);
  deviceID = WiFi.macAddress();
  deviceID.replace(":", "");
  configuraWiFi();
  recuperaHorasNav(); // Función para recuperar h_nav desde la hoja de Google
}

void loop() {
  if (millis() - lastSampleTime >= samplingInterval) {
    lastSampleTime = millis();

    muestraDebugGPS();

    if (gps.location.isValid()) {
      float knots = gps.speed.kmph() * 0.539957;
      if (knots > 0.2) {
        if (lastNavUpdateTime == 0) { // Primera vez que se supera el umbral
          lastNavUpdateTime = millis();
        } else {
          // Sumar el tiempo transcurrido al tiempo total de navegación
          h_nav += (millis() - lastNavUpdateTime) / 3600000.0; // Convertir milisegundos a horas
          lastNavUpdateTime = millis();
        }
      } else {
        lastNavUpdateTime = 0; // Resetear el timer cuando la velocidad cae debajo del umbral
      }
      enviaDatosGPS();
    } else {
      Serial.println("GPS: Buscando señal...");
    }
  }

  verificaYReconectaWiFi();
}

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
                       "&sat=" + gps.satellites.value() +
                       "&h_nav=" + String(h_nav, 6); // Asegurarse de enviar con precisión adecuada
  return queryString;
}

// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17);
  deviceID = WiFi.macAddress();
  deviceID.replace(":", "");
  configuraWiFi();
}

void loop() {
  
  if (millis() - lastSampleTime >= samplingInterval) {
    lastSampleTime = millis();

    muestraDebugGPS();

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
// Esta función muestra información de debugging del GPS por el puerto serial.
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
  // String fullURL = "https://script.google.com/macros/s/AKfycby4lY8-W-a6qcFBgNJUQaU2ouxGk4JYRj-OXXX9RdMHwPqtXC-m_PNR_3tRNeye_IQy/exec" + queryString;
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

// Esta función verifica y, si es necesario, intenta reconectar el WiFi usando la configuración de múltiples redes.
void verificaYReconectaWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, intentando reconectar...");
    configuraWiFi(); // Llama a la función que maneja la conexión a múltiples redes.
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.println("Reconectado a WiFi");
    } else {
      Serial.println("No se pudo reconectar a ninguna red.");
    }
  }
}

#include <HTTPClient.h>

void recuperaHorasNav() {
  HTTPClient http;
    String serverPath = "https://script.google.com/macros/s/AKfycbwaVPUGitFwHKSnv_h4NUZsSus1Bfjsk0sWI80vwDO6PO32qmqonAar_akHxPKiR3HU/exec";

  // Inicia una conexión al servidor
  http.begin(serverPath);

  // Realiza la solicitud HTTP GET
  int httpCode = http.GET();

  if (httpCode > 0) { // Comprueba si la solicitud fue exitosa
    String payload = http.getString();
    h_nav = payload.toFloat(); // Asegúrate de que el valor devuelto es convertido correctamente a float
    Serial.println("Valor recuperado de h_nav: " + String(h_nav));
  } else {
    Serial.println("Error al recuperar h_nav: " + http.errorToString(httpCode));
  }

  // Cierra la conexión
  http.end();
}

