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
#define TOKEN_SUNRISE "T9uMOFBfFJDinGWUbvjzefmnmKZGwVlK"
#define TOKEN_SUNSET "W9s_EDdsHpJCoHy5ysyDXBk4xD3F-zJB"
#define TOKEN_SUNSET_ICE "S2OyggnDcBS9ywbj2s3h4jUPAY7whY4l"
#define TOKEN_MOONSET "j9MTrEHHxL-PHMrlMpdBPkt8z8_MfPca"
#define TOKEN_SBLU "u_sVqqznQcOi_2jmXeJbKVDHsbQeMyUd"
#define TOKEN_TRINIDAD "-oIB8frVsht-bDkqVysb8PfEarxNhp-4"
#define TOKEN_DEFAULT "SSfjatyAs3rPdFGheg5GScCQajNinV11"

// Definición de direcciones MAC
#define MAC_SUNRISE "34:98:7A:6C:65:D8"
#define MAC_SUNSET_ICE "08:F9:E0:CE:A3:F4"
#define MAC_MOONSET "34:98:7A:6C:65:54"
#define MAC_SUNSET "34:98:7A:6C:65:78"
#define MAC_SBLU "34:98:7A:6C:67:80"

#define MAC_TRINIDAD "YY:YY:YY:YY:YY:YY"
#define MAC_DEFAULT "XX:YY:YY:YY:YY:YY"

// Battery and USB detection pin
#define BATTERY_PIN 35  // GPIO35 is used for battery level measurement
#define USB_POWER_PIN 34  // GPIO34 is used for USB power detection

// Función para configurar Blynk según la MAC del dispositivo
String deviceID = "NNNN-TBEAM";  // Valor por defecto
String clientID = "NNNN-MFA";  // Valor por defecto

const char* token = TOKEN_DEFAULT;
const char* CERTIFICATE;
const char* PRIVATE_KEY;
const char* ROOT_CA;

void configurarBlynk() {
    String macAddress = WiFi.macAddress();

    if (macAddress.equalsIgnoreCase(MAC_SUNRISE)) {
        token = TOKEN_SUNRISE;
        deviceID = "SNRS-TBEAM";
        clientID  = "AMO-MFA";
        CERTIFICATE = cert_SNRS_TBEAM;
        PRIVATE_KEY = key_SNRS_TBEAM;
        ROOT_CA = ca_SNRS_TBEAM;

    } else if (macAddress.equalsIgnoreCase(MAC_SUNSET)) {
        token = TOKEN_SUNSET;
        deviceID = "SNST-TBEAM";
        clientID  = "AMO-MFA";
        CERTIFICATE = cert_SNST_TBEAM;
        PRIVATE_KEY = key_SNST_TBEAM;
        ROOT_CA = ca_SNST_TBEAM;

    } else if (macAddress.equalsIgnoreCase(MAC_SUNSET_ICE)) {
        token = TOKEN_SUNSET_ICE;
        deviceID = "SICE-TBEAM";
        clientID  = "GRM-MFA";
        CERTIFICATE = cert_SICE_TBEAM;
        PRIVATE_KEY = key_SICE_TBEAM;
        ROOT_CA = ca_SICE_TBEAM;

    } else if (macAddress.equalsIgnoreCase(MAC_MOONSET)) {
        token = TOKEN_MOONSET;
        deviceID = "MNST-TBEAM";
        clientID  = "GRM-MFA";
        CERTIFICATE = cert_MNST_TBEAM;
        PRIVATE_KEY = key_MNST_TBEAM;
        ROOT_CA = ca_MNST_TBEAM;

    } else if (macAddress.equalsIgnoreCase(MAC_SBLU)) {
        token = TOKEN_SBLU;
        deviceID = "SBLU-TBEAM";
        clientID  = "GRM-MFA";
        CERTIFICATE = cert_SBLU_TBEAM;
        PRIVATE_KEY = key_SBLU_TBEAM;
        ROOT_CA = ca_SBLU_TBEAM;

    } else if (macAddress.equalsIgnoreCase(MAC_TRINIDAD)) {
        token = TOKEN_TRINIDAD;
        deviceID = "TRND-TBEAM";
        clientID  = "JLC-MFA";
        CERTIFICATE = cert_TRND_TBEAM;
        PRIVATE_KEY = key_SBLU_TBEAM;
        ROOT_CA = ca_SBLU_TBEAM; 
    }

    Serial.print("Configuring Blynk with device: ");
    Serial.print(deviceID);
    Serial.print(", MAC: ");
    Serial.println(macAddress);

    Blynk.config(token);
    Blynk.connect();
}

#endif // CONFIG_H
