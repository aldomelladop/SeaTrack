#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
// const char* ssids[] = {"Marina Fiordo Aysen","Redmi Note 12","STARLINK", "Sunrise", "Gatitos 2.4","Sunset Ice"};
const char* ssids[] = {"Sunset Ice", "Marina Fiordo Aysen","Redmi Note 12","STARLINK", "Sunrise", "Gatitos 2.4"};
const char* passwords[] = {"Sunsetice2024", "Duke2023","qpwo1029","", "Sunrise2024", "18412044"};

// OTA Configuration
const char* HOSTNAME = "TBEAM-GPS-Device";
const char* OTA_PASSWORD = "MFA_29052024"; // contraseña segura 


// Server URL for HTTP requests
const char* SERVER_URL = "https://script.google.com/macros/s/AKfycbzIeSKDUaIP-3TIvQAVne6sLLvPvaarxSFdMtbFUn0PEQEQ3WhkqEJb390UxrjwL6DLiw/exec";

// Auth Token de Blynk
#define BLYNK_TEMPLATE_ID "TMPL2xv9_DR2h"
#define BLYNK_TEMPLATE_NAME "SeaTrack Dashboard"
char auth[] = "N-dURNFSPDyHSLugM16D63tLIsXMfeUX";  // Token SUNRISE
// char auth[] = "SSfjatyAs3rPdFGheg5GScCQajNinV0s";  // Token TRINIDAD
// char auth[] = "pkBo91IkD1Jed3pJ42LDsXG42J7JJFdl";  // Token SUNSET
// char auth[] = "iNvkzmlZg_GflPlU6JY5_dHlKJ0hxsaW";  // Token SUNSET-ICE


#endif // CONFIG_H
