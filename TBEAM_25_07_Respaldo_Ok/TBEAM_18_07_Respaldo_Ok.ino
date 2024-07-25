#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "config.h"
#include "LoRaBoards.h"
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
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <pgmspace.h>


WiFiMulti wifiMulti;
ESP32Time rtc;
const unsigned long GPS_UPDATE_INTERVAL = 300000;
const unsigned long DATA_SEND_INTERVAL = 600000;
unsigned long lastGPSUpdateTime = 0;
unsigned long lastDataSendTime = 0;

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

bool wifiConnected = false;
const unsigned long samplingInterval = 60000;
unsigned long lastSampleTime = 0;
float h_nav = 0;
unsigned long lastNavUpdateTime = 0;
unsigned long lastGPSFixTime = 0;
bool gpsFix = false;

float filteredLat = 0.0;
float filteredLng = 0.0;

WiFiClientSecure net;
PubSubClient client(net);

#define GPS_MODEL "NEO-M8N"  // Definir el modelo del GPS aquí


void configuraWiFi();
void setupOTA();
void muestraDebugGPS();
void checkGPSFix();
String generaJSONPayload();
void enviaDatosGPS();
void verificaYReconectaWiFi();
void verificaIntervalosYReinicia();
void sincronizaTiempo();
void enviarDatosATiempo();
void obtenerTiempo(struct tm& timeinfo);
void enviarDatosPuertos();
void muestraEstadoBateria();

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Wire.begin();

  if (!beginPower()) {
    Serial.println("Fallo al inicializar PMU");
    while (1)
      ;
  }

  configuraWiFi();
  setupOTA();
  sincronizaTiempo();
  configurarBlynk();

  net.setCACert(ROOT_CA);
  net.setCertificate(CERTIFICATE);
  net.setPrivateKey(PRIVATE_KEY);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(callback);

  if (client.connect(THING_NAME)) {
    Serial.println("Connected to AWS IOT");
  } else {
    Serial.println("AWS IoT Timeout!");
  }
}

void loop() {
  ArduinoOTA.handle();
  Blynk.run();
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastSampleTime >= samplingInterval) {
    lastSampleTime = currentMillis;
    Serial.println("Tomando muestra del GPS...");
    muestraDebugGPS();
    checkGPSFix();
    if (gps.location.isValid() && gpsFix) {
      enviaDatosGPS();
      enviarDatosATiempo();

      String coordenadas = String(filteredLat, 6) + ", " + String(filteredLng, 6);
      Blynk.virtualWrite(V14, coordenadas);

      struct tm timeinfo;
      obtenerTiempo(timeinfo);
      char fechaHora[20];
      strftime(fechaHora, sizeof(fechaHora), "%d-%m-%y - %H:%M", &timeinfo);
      Blynk.virtualWrite(V15, fechaHora);

      muestraEstadoBateria();

      if (PMU->isCharging() || PMU->isVbusIn()) {
        delay(15000);
        esp_sleep_enable_timer_wakeup(105 * 1000000);  // 1 min 45 sec
      } else {
        delay(15000);
        esp_sleep_enable_timer_wakeup(225 * 1000000);  // 3 min 45 sec
      }
      esp_deep_sleep_start();
    } else {
      Serial.println("GPS: Buscando señal...");
    }
  }
  verificaYReconectaWiFi();
  verificaIntervalosYReinicia();
}

void muestraDebugGPS() {
  esp_task_wdt_reset();  // Reiniciar el Watchdog Timer
  while (GPSSerial.available() > 0) {
    char c = GPSSerial.read();
    if (gps.encode(c)) {
      filteredLat = gps.location.lat();
      filteredLng = gps.location.lng();

      Serial.printf("GPS: Lat = %f, Lng = %f\n", filteredLat, filteredLng);
      Serial.printf("Satellites: %d, HDOP: %f, Speed: %f, Course: %f\n",
                    gps.satellites.value(), gps.hdop.hdop(), gps.speed.kmph(), gps.course.deg());
    }
  }
}



String generaJSONPayload() {
  StaticJsonDocument<512> doc;
  time_t local = rtc.getEpoch();
  struct tm timeinfo;
  localtime_r(&local, &timeinfo);
  doc["clientID"] = clientID;
  doc["deviceID"] = deviceID;
  doc["lat"] = filteredLat;
  doc["lng"] = filteredLng;
  doc["year"] = timeinfo.tm_year + 1900;
  doc["month"] = timeinfo.tm_mon + 1;
  doc["day"] = timeinfo.tm_mday;
  doc["hour"] = timeinfo.tm_hour;
  doc["minute"] = timeinfo.tm_min;
  doc["second"] = timeinfo.tm_sec;
  doc["speed"] = gps.speed.kmph();
  doc["sat"] = gps.satellites.value();
  doc["knots"] = gps.speed.knots();
  doc["course"] = gps.course.deg();
  doc["h_nav"] = h_nav;
  doc["hdop"] = gps.hdop.value();

  String output;
  serializeJson(doc, output);
  return output;
}

void enviaDatosGPS() {
  String payload = generaJSONPayload();
  client.publish("sensorDevice/data", payload.c_str());
  Serial.println("Datos enviados a AWS IoT: ");
  Serial.println(payload);
}
void configuraWiFi() {
  for (int i = 0; i < sizeof(ssids) / sizeof(ssids[0]); i++) {
    wifiMulti.addAP(ssids[i], passwords[i]);
  }

  Serial.println("Conectando a WiFi...");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado.");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupOTA() {
  ArduinoOTA.setHostname(HOSTNAME);

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
    while (wifiMulti.run() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nReconectado a WiFi.");
  }
}

void verificaIntervalosYReinicia() {
  unsigned long currentTime = millis();
  if ((currentTime - lastGPSUpdateTime > GPS_UPDATE_INTERVAL) || (currentTime - lastDataSendTime > DATA_SEND_INTERVAL)) {
    Serial.println("Reinicio necesario: intervalos máximos excedidos sin actualización del GPS o envío de datos.");
    ESP.restart();
  }
}

void checkGPSFix() {
  if (gps.location.isValid()) {
    lastGPSFixTime = millis();
    gpsFix = true;
    Serial.println("GPS fix adquirido.");
  } else if (millis() - lastGPSFixTime > 300000) {
    gpsFix = false;
    Serial.println("GPS fix perdido por más de 5 minutos!");
  }
}

void sincronizaTiempo() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  setenv("TZ", "CLT4CLST,M10.2.0/00:00:00,M3.2.0/00:00:00", 1);
  tzset();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Fallo al sincronizar el tiempo con NTP");
    return;
  }
  rtc.setTimeStruct(timeinfo);

  Serial.println("Tiempo sincronizado con NTP:");
  Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
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
  Blynk.virtualWrite(V9, deviceID);
  Blynk.virtualWrite(V10, String(filteredLat, 6));
  Blynk.virtualWrite(V11, String(filteredLng, 6));
}

void obtenerTiempo(struct tm& timeinfo) {
  time_t local = rtc.getEpoch();
  localtime_r(&local, &timeinfo);
}

void muestraEstadoBateria() {
  Serial.print("isCharging:");
  Serial.println(PMU->isCharging() ? "YES" : "NO");
  Serial.print("isDischarge:");
  Serial.println(PMU->isDischarge() ? "YES" : "NO");
  Serial.print("isVbusIn:");
  Serial.println(PMU->isVbusIn() ? "YES" : "NO");
  Serial.print("getBattVoltage:");
  Serial.print(PMU->getBattVoltage());
  Serial.println("mV");
  Serial.print("getVbusVoltage:");
  Serial.print(PMU->getVbusVoltage());
  Serial.println("mV");
  Serial.print("getSystemVoltage:");
  Serial.print(PMU->getSystemVoltage());
  Serial.println("mV");
  if (PMU->isBatteryConnect()) {
    Serial.print("getBatteryPercent:");
    Serial.print(PMU->getBatteryPercent());
    Serial.println("%");
    Blynk.virtualWrite(V12, PMU->getBatteryPercent());
  }
  bool usbConnected = PMU->isVbusIn();
  Serial.print("USB Connected: ");
  Serial.println(usbConnected ? "YES" : "NO");
  Blynk.virtualWrite(V13, usbConnected ? "Sí" : "No");  // Enviar estado del USB a Blynk

  Serial.println();
}

void setupGPS() {
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  if (strcmp(GPS_MODEL, "NEO-M8N") == 0) {
    Serial.println("Using NEO-M8N GNSS receiver");
    configNEOM8N();
  } else if (strcmp(GPS_MODEL, "NEO-M6N") == 0) {
    Serial.println("Using NEO-M6N GNSS receiver");
    configNEOM6N();
  } else {
    Serial.println("GPS Model not recognized!");
  }
}

void configNEOM8N() {
  GPSSerial.write("$PMTK251,9600*17\r\n");                                   // Configurar la tasa de baudios
  GPSSerial.write("$PMTK220,1000*1F\r\n");                                   // Configurar la frecuencia de actualización
  GPSSerial.write("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");  // Configurar las sentencias NMEA
}

void configNEOM6N() {
  GPSSerial.write("$PMTK251,9600*17\r\n");                                   // Configurar la tasa de baudios
  GPSSerial.write("$PMTK220,200*2C\r\n");                                    // Configurar la frecuencia de actualización
  GPSSerial.write("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");  // Configurar las sentencias NMEA
}
