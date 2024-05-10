#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <ArduinoOTA.h> // Incluir la biblioteca OTA

// Declaraciones de funciones
void configuraWiFi();
void muestraDebugGPS();
void enviaDatosGPS();
String generaQueryString();
void verificaYReconectaWiFi();
void setupOTA();

// Configuración WiFi
const char* ssids[] = {"Redmi Note 12", "Sunrise", "Gatitos 2.4"};
const char* passwords[] = {"qpwo1029", "Sunrise2024", "18412044"};
int numRedes = 4; // Número de redes disponibles

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

void setup() {
  Serial.begin(115200);
  // GPSSerial.begin(9600, SERIAL_8N1, 16, 17); // esp32
  GPSSerial.begin(9600, SERIAL_8N1, 34, 12); // Usar pines 34 y 12 para GPS RX y TX respectivamente. (Lilygo)

  deviceID = WiFi.macAddress();
  deviceID.replace(":", "");
  configuraWiFi();
  setupOTA(); // Configura y inicia OTA
}

void loop() {
  ArduinoOTA.handle(); // Manejar cualquier potencial actualización OTA

  if (millis() - lastSampleTime >= samplingInterval) {
    lastSampleTime = millis();


    muestraDebugGPS();

    if (gps.location.isValid()) {
      float knots = gps.speed.kmph() * 0.539957;
      if (knots > 0.2) {
        if (lastNavUpdateTime == 0) { // Primera vez que se supera el umbral
          lastNavUpdateTime = millis();
        } else {
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

void setupOTA() {
  // Hostname predeterminado es "esp32-<ChipID>", pero puede ser modificado
  ArduinoOTA.setHostname("ESP32-GPS-Device");

  // Sin contraseña
  // ArduinoOTA.setPassword("admin");

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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

// ------------------------------------------------------------
//  Librerías de funciones
// ------------------------------------------------------------

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
                       "&sat=" + gps.satellites.value() +
                       "&h_nav=" + String(h_nav, 6); // Asegurarse de enviar con precisión adecuada
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