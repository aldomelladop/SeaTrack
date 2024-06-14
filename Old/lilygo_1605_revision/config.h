#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
const char* ssids[] = {"STARLINK","Redmi Note 12", "Sunrise", "Gatitos 2.4"};
const char* passwords[] = {"", "qpwo1029", "Sunrise2024", "18412044"};

// OTA Configuration
const char* HOSTNAME = "ESP32-GPS-Device";

// Definiciones de plantilla Blynk
#define BLYNK_TEMPLATE_ID "TMPL2xv9_DR2h"
#define BLYNK_TEMPLATE_NAME "SeaTrack Dashboard"

// Token de autenticación Blynk
const char auth[] = "0B26LMjKWEO9bHsXpeIiRaRzEZMiqMe5";  // Reemplaza con tu token de Blynk

// Server URL for HTTP requests
const char* SERVER_URL = "https://script.google.com/macros/s/AKfycbzIeSKDUaIP-3TIvQAVne6sLLvPvaarxSFdMtbFUn0PEQEQ3WhkqEJb390UxrjwL6DLiw/exec";

#endif // CONFIG_H
