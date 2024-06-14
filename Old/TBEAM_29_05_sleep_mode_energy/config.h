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

// Server URL for HTTP requests
const char* SERVER_URL = "https://script.google.com/macros/s/AKfycbzIeSKDUaIP-3TIvQAVne6sLLvPvaarxSFdMtbFUn0PEQEQ3WhkqEJb390UxrjwL6DLiw/exec";

// Auth Token de Blynk
#define BLYNK_TEMPLATE_ID "TMPL2xv9_DR2h"
#define BLYNK_TEMPLATE_NAME "SeaTrack Dashboard"

// Definición de tokens de Blynk
#define TOKEN_SUNRISE "N_dURNFSPDyHSLugM16D63tILsXMfeUX"
#define TOKEN_TRINIDAD "SSFjatyAs3rPdFGheg5SGC0qjNinVoS"
#define TOKEN_SUNSET "pkBo91IkD1Jed3pJ24LDsXG42J7JJFd1"
#define TOKEN_SUNSET_ICE "iNvkzmlZg_Gf1P1U6JY5_dH1KJ0hxsAW"
#define TOKEN_DEFAULT "default_token_here"

// Definición de direcciones MAC
#define MAC_SUNRISE "34:98:7A:6C:67:80" // Reemplazar con la MAC real para SUNRISE
#define MAC_TRINIDAD "YY:YY:YY:YY:YY:YY" // Reemplazar con la MAC real para TRINIDAD
#define MAC_SUNSET "ZZ:ZZ:ZZ:ZZ:ZZ:ZZ" // Reemplazar con la MAC real para SUNSET
#define MAC_SUNSET_ICE "34:98:7A:6C:65:78" // Reemplazar con la MAC real para SUNSET-ICE

// Battery and USB detection pin
#define BATTERY_PIN 35  // GPIO35 is used for battery level measurement
#define USB_POWER_PIN 34  // GPIO34 is used for USB power detection

#endif // CONFIG_H
