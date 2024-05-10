#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

// Declaraciones de funciones
void configuraWiFi();
void muestraDebugGPS();
void enviaDatosGPS();
void verificaYReconectaWiFi();
void setupOTA();

// Configuración WiFi - Lista de redes WiFi conocidas
const char* ssids[] = {"Redmi Note 12", "Sunrise", "Gatitos 2.4"};
const char* passwords[] = {"qpwo1029", "Sunrise2024", "18412044"};
int numRedes = 3;

// Instancias y variables globales para el GPS
TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

// Identificador único del dispositivo (dirección MAC)
char deviceID[13]; // Usamos char array en lugar de String para mejor manejo de memoria

// Flag de conexión a WiFi
bool wifiConnected = false;

// Configuración de zona horaria para marcar correctamente el tiempo de los eventos del GPS
TimeChangeRule myDST = {"CLST", Last, Sun, Oct, 3, -180}; // Horario de verano
TimeChangeRule mySTD = {"CLT", Last, Sun, Mar, 3, -240}; // Horario estándar
Timezone myTZ(myDST, mySTD);

// Intervalo de muestreo para leer datos del GPS
const unsigned long samplingInterval = 60000; // 1 minuto entre actualizaciones
unsigned long lastSampleTime = 0;

// Datos de navegación
float h_nav = 0; // Horas de navegación en horas
unsigned long lastNavUpdateTime = 0; // Para marcar la última actualización cuando la lancha navega

// Credenciales de Firebase
#define FIREBASE_HOST "https://seatrack-909bd-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "tM8an1PKVK2jSnvci9uhasT4RcR5OzQCJXOo6KI2"

FirebaseData firebaseData;

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, 34, 12); // Usar pines 34 y 12 para GPS RX y TX respectivamente. (Lilygo)
  
  // Configura y conecta a WiFi
  configuraWiFi();

  // Inicia la conexión con Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  
  // Configura las funcionalidades de OTA (Actualización por aire)
  setupOTA();

  // Configura el identificador único del dispositivo (MAC Address)
  strncpy(deviceID, WiFi.macAddress().c_str(), sizeof(deviceID));
  for (char* p = deviceID; *p; p++) { // Elimina los ':' para tener un ID más limpio
    if (*p == ':') {
      memmove(p, p + 1, strlen(p));
    }
  }
}

void loop() {
  // Maneja las actualizaciones OTA
  ArduinoOTA.handle();

  // Ejecuta cada minuto
  if (millis() - lastSampleTime >= samplingInterval) {
    lastSampleTime = millis();

    // Muestra la información del GPS
    muestraDebugGPS();

    // Si la ubicación del GPS es válida, envía los datos
    if (gps.location.isValid()) {
      enviaDatosGPS();
    } else {
      Serial.println("GPS: Buscando señal...");
    }
  }

  // Verifica y reconecta WiFi si es necesario
  verificaYReconectaWiFi();
}

void configuraWiFi() {
  Serial.println("Conectando a WiFi...");
  // Intenta conectarse a cada red WiFi en la lista
  for (int i = 0; i < numRedes; i++) {
    WiFi.begin(ssids[i], passwords[i]);
    Serial.print("Intentando conectar a: ");
    Serial.println(ssids[i]);

    // Espera hasta que se conecte a la red WiFi o se agoten los intentos
    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 10) {
      delay(500);
      Serial.print(".");
      intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi conectado.");
      wifiConnected = true;
      break; // Sale del bucle si se conecta exitosamente
    } else {
      Serial.println("\nFallo en la conexión. Intentando con la siguiente red...");
    }
  }

  if (!wifiConnected) {
    Serial.println("No se pudo conectar a ninguna red. Revisa tus credenciales.");
  }
}

void muestraDebugGPS() {
  // Muestra datos del GPS por el puerto serial para depuración
  while (GPSSerial.available() > 0) {
    char c = GPSSerial.read();
    if (gps.encode(c)) {
      // Si se ha recibido una nueva sentencia válida del GPS
      Serial.print("GPS: ");
      Serial.print("Latitud: "); Serial.print(gps.location.lat(), 6);
      Serial.print(", Longitud: "); Serial.print(gps.location.lng(), 6);
      Serial.print(" Satélites: "); Serial.print(gps.satellites.value());
      Serial.println();
    }
  }
}

void enviaDatosGPS() {
  if (gps.location.isValid()) {
    // Formatear la fecha y hora al estándar ISO 8601
    char timestamp[25];
    snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02dT%02d:%02d:%02dZ",
             gps.date.year(), gps.date.month(), gps.date.day(),
             gps.time.hour(), gps.time.minute(), gps.time.second());

    // Crear el objeto JSON con los datos
    FirebaseJson json;
    json.set("lat", gps.location.lat());
    json.set("lng", gps.location.lng());
    json.set("speed", gps.speed.kmph());
    json.set("sat", gps.satellites.value());
    json.set("knots", gps.speed.kmph() * 0.539957);

    // Crear la ruta en la base de datos
    String path = "/deviceReadings/" + String(deviceID) + "/" + timestamp;

    // Enviar los datos a Firebase
    if (Firebase.pushJSON(firebaseData, path, json)) {
      Serial.println("Datos enviados a Firebase");
    } else {
      Serial.println("Error enviando datos: " + firebaseData.errorReason());
    }
  } else {
    Serial.println("Datos de GPS no válidos, no se envían a Firebase");
  }
}

void verificaYReconectaWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, intentando reconectar...");
    configuraWiFi();
  }
}

void setupOTA() {
  ArduinoOTA.setHostname("ESP32-GPS-Device");
  ArduinoOTA.setPassword("admin"); // Establecer tu propia contraseña para mayor seguridad
  ArduinoOTA.setPort(8267); // Cambio del puerto por uno menos convencional

  ArduinoOTA.onStart([]() {
    Serial.println("Iniciando actualización OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nActualización OTA finalizada");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progreso: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Fallo de autenticación");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Fallo al comenzar");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Fallo de conexión");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Fallo al recibir");
    else if (error == OTA_END_ERROR) Serial.println("Fallo al finalizar");
  });
  ArduinoOTA.begin();
}
