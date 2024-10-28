#ifndef CONFIG_H
#define CONFIG_H

// Auth Token de Blynk
#define BLYNK_TEMPLATE_ID "TMPL2xv9_DR2h"
#define BLYNK_TEMPLATE_NAME "SeaTrack Dashboard"

#define THING_NAME "sensorDevice/data"
#define AWS_IOT_ENDPOINT "a2z3up0z3g2dc0-ats.iot.us-east-2.amazonaws.com"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "certificates.h"  // Incluir el archivo de certificados

// WiFi Credentials
const char* ssids[] = {
    "Sunset Ice", 
    "Marina Fiordo Aysen 2.4", 
    "Redmi Note 12", 
    "STARLINK", 
    "Sunrise", 
    "Gatitos 2.4",
    "Moonset",
    "iPhone"
};

const char* passwords[] = {
    "Sunsetice2024", 
    "Duke2023", 
    "qpwo1029", 
    "", 
    "Sunrise2024", 
    "18412044",
    "Moonset2024",
    "1029qpwo"
};


// OTA Configuration
const char* HOSTNAME = "TBEAM-GPS-Device";
const char* OTA_PASSWORD = "MFA_29052024"; // Contraseña segura

// Definición de tokens de Blynk
#define TOKEN_SUNSET_ICE "S2OyggnDcBS9ywbj2s3h4jUPAY7whY4l"

// Definición de direcciones MAC
#define MAC_SUNSET_ICE "08:F9:E0:CE:A3:F4"

// Battery and USB detection pin
#define BATTERY_PIN 35  // GPIO35 is used for battery level measurement
#define USB_POWER_PIN 34  // GPIO34 is used for USB power detection

// Función para configurar Blynk según la MAC del dispositivo
String deviceID = "NNNN-TBEAM";  // Valor por defecto
String clientID = "NNNN-MFA";  // Valor por defecto

const char* token = "TOKEN_DEFAULT";
const char* CERTIFICATE;
const char* PRIVATE_KEY;
const char* ROOT_CA;

void configurarBlynk() {
    String macAddress = WiFi.macAddress();

    token = TOKEN_SUNSET_ICE;
    deviceID = "SICE-TBEAM";
    clientID  = "GRM-MFA";
    CERTIFICATE = cert_SICE_TBEAM;
    PRIVATE_KEY = key_SICE_TBEAM;
    ROOT_CA = ca_SICE_TBEAM;

    Serial.print("Configuring Blynk with device: ");
    Serial.print(deviceID);
    Serial.print(", MAC: ");
    Serial.println(macAddress);

    Blynk.config(token);
    Blynk.connect();
}

#endif // CONFIG_H
