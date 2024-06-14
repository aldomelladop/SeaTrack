/**
 * @file      config.cpp
 * @author    aldomellado (aldo.mellado@fiordoaysen.cl)
 * @date      2024-06-05
 * @note      .
 */


#include "config.h"

// WiFi Credentials
const char* ssids[NUM_SSIDS] = {"Sunset Ice", "Marina Fiordo Aysen","Redmi Note 12","STARLINK", "Sunrise", "Gatitos 2.4"};
const char* passwords[NUM_SSIDS] = {"Sunsetice2024", "Duke2023","qpwo1029","", "Sunrise2024", "18412044"};

// OTA Configuration
const char* HOSTNAME = "TBEAM-GPS-Device";
const char* OTA_PASSWORD = "MFA_29052024";

// Server URL for HTTP requests
const char* SERVER_URL = "https://script.google.com/macros/s/AKfycbzIeSKDUaIP-3TIvQAVne6sLLvPvaarxSFdMtbFUn0PEQEQ3WhkqEJb390UxrjwL6DLiw/exec";

// Auth Token de Blynk
char auth[] = "N-dURNFSPDyHSLugM16D63tLIsXMfeUX";
