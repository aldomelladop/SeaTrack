#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
const char* ssids[] = {
    "Sunset Ice", 
    "Marina Fiordo Aysen", 
    "Redmi Note 12", 
    "STARLINK", 
    "Sunrise", 
    "Gatitos 2.4"
};

const char* passwords[] = {
    "Sunsetice2024", 
    "Duke2023", 
    "qpwo1029", 
    "", 
    "Sunrise2024", 
    "18412044"
};

// OTA Configuration
const char* HOSTNAME = "TBEAM-GPS-Device";
const char* OTA_PASSWORD = "MFA_29052024"; // Contraseña segura

// Server URLs for HTTP requests
const char* SERVER_URL = "https://script.google.com/macros/s/AKfycbzIeSKDUaIP-3TIvQAVne6sLLvPvaarxSFdMtbFUn0PEQEQ3WhkqEJb390UxrjwL6DLiw/exec"; // Cambia esto a tu URL de servidor
const char* PORT_URL = "https://script.google.com/macros/s/AKfycbyIGk-H4p1qjDOaGnln8muPg-Pjx9Jc6kGc1VJVPMdepe8R8hBcrvqDr44iYg92IU-k/exec";

// Auth Token de Blynk
#define BLYNK_TEMPLATE_ID "TMPL2xv9_DR2h"
#define BLYNK_TEMPLATE_NAME "SeaTrack Dashboard"

// Definición de tokens de Blynk
#define TOKEN_SUNRISE "N-dURNFSPDyHSLugM16D63tLIsXMfeUX"
#define TOKEN_TRINIDAD "SSfjatyAs3rPdFGheg5GScCQajNinV0s"
#define TOKEN_SUNSET "pkBo91IkD1Jed3pJ42LDsXG42J7JJFdl"
#define TOKEN_SUNSET_ICE "iNvkzmlZg_GflPlU6JY5_dHlKJ0hxsaW"
#define TOKEN_DEFAULT "8IBclkkw_CF1MMwsl-eBvzSvOXljVjSV"

// Definición de direcciones MAC
#define MAC_SUNRISE "08:F9:E0:F5:95:9C"
#define MAC_TRINIDAD "YY:YY:YY:YY:YY:YY"
#define MAC_SUNSET "34:98:7A:6C:65:54"
#define MAC_MOONSET "34:98:7A:6C:65:78"
#define MAC_SUNSET_ICE "78:65:6C:7A:98:34"

// Battery and USB detection pin
#define BATTERY_PIN 35  // GPIO35 is used for battery level measurement
#define USB_POWER_PIN 34  // GPIO34 is used for USB power detection

#endif // CONFIG_H
