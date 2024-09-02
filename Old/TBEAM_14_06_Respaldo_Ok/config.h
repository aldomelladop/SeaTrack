#ifndef CONFIG_H
#define CONFIG_H

// Auth Token de Blynk
#define BLYNK_TEMPLATE_ID "TMPL2xv9_DR2h"
#define BLYNK_TEMPLATE_NAME "SeaTrack Dashboard"

#define THING_NAME "SensorDevice"
#define AWS_IOT_ENDPOINT "a2z3up0z3g2dc0-ats.iot.us-east-2.amazonaws.com"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

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


// Certificados de Amazon
const char* ROOT_CA PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

const char* CERTIFICATE PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUTqYp6+UU3vstaNbEUIfcviaQU2owDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI0MDYxMjE3MzU0
NVoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAM3WS7Ycm5U0JA6jo/8G
R5hVa31QUKxuPa6xBD+fWEZmxOHl0FyBee80dLlZ169Zw5vgPmXYXFHqOei3kkxP
x7/bQvsvwKGJWjH/GLkP8PaCEqZYEReMd6fx2KOd9IhRdhNOargNmOu24nb2UdDk
asqpxxVPSOwwTb6wh/7X6/3/s7sGyKsBIIThvNj3b7wSyhN2gx/4OspEdQTVfJqI
jdBiJ3CxpiXp+UdrWT9It40rB85x9gYu8lA80fEr9jlv6lv8B/VPYLmWe4v+MNis
YllaQMMSAezk6xgz08S5iawCarJZLuBrJ9u2ljxv/m3K/4aZ9F/dYMz9y4cHBtFT
14cCAwEAAaNgMF4wHwYDVR0jBBgwFoAU/ZDD1m4sCm5ARPTBB5DhEyTY81EwHQYD
VR0OBBYEFIorgWU+74soylNyS7gw9jiJm3MbMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBmeDoqdp5znBNvQ1+n1Z6QWiIW
Ob96Iedy9YUeau7/jbCuneCFpZpajThPjZUU5j9HA4LH3qLf3IjeQAZpC85mI+8h
MVa2WqIB/VTsaQPb31BgUxLPP66DyrT4q1/XhGZiL6ZkfjIuQmLJaddKPFaDZ7G3
DZaOg4YvZ0VmJCnFGX2P3xm4R66bqEp6UIHdHTEmUdo1R5kRlB22cdFJ9v2PngKc
Bx4HTkH27xQwc9CA+mfWpMIEMQglMiPicFW1QcwfTldUCTPweEDqbznD6swTUTG2
iZbtNMRtW5A0hnHwKtMsfhV5XrW4RkCZxRzUCeqctvf5jBDy1Lq+3478F1aF
-----END CERTIFICATE-----
)EOF";

const char* PRIVATE_KEY PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpgIBAAKCAQEAzdZLthyblTQkDqOj/wZHmFVrfVBQrG49rrEEP59YRmbE4eXQ
XIF57zR0uVnXr1nDm+A+ZdhcUeo56LeSTE/Hv9tC+y/AoYlaMf8YuQ/w9oISplgR
F4x3p/HYo530iFF2E05quA2Y67bidvZR0ORqyqnHFU9I7DBNvrCH/tfr/f+zuwbI
qwEghOG82PdvvBLKE3aDH/g6ykR1BNV8moiN0GIncLGmJen5R2tZP0i3jSsHznH2
Bi7yUDzR8Sv2OW/qW/wH9U9guZZ7i/4w2KxiWVpAwxIB7OTrGDPTxLmJrAJqslku
4Gsn27aWPG/+bcr/hpn0X91gzP3LhwcG0VPXhwIDAQABAoIBAQCEvRhQFXzDqD9E
bYT5lfwfoZ5SzrNnryP5/s+lk8WiiO2YW9KVhS2z85j+WpC71OBPKjozBWbrsGet
yov+yEghUm2MfMv1oBTZPw+bcEtNPK54NTy4ee5EELNLODaUnivN4XjyvloKUAMV
Al3anyK7FOd8t4doQPIx8beRwX6/abk9/MlrpeCt+xLk4ZJIb/UA5pqlzpaPWpaQ
+T4deWy15uFOnfJUB/WzDaIK2JluicYMjLiOmSIuYdjc2v0yiSoF47fhPW3KvjnZ
T3lVtAQu+shCp7jb/l0o6SigPkXY7/io0XGtSD8nBXkghXYpi+NA3I7kyMekOnh/
PTowwbEBAoGBAOtVcHWAPJfAqrrXDXJss0o6mGFmTv8fyw9zW0VdJI+aD7e9tM0i
FcNpG/Vjrq2nTrigeAQGUoc7Lzf3OyuuhjYzROdzFcfRSb2pTrsoge5Vf9Pst+Yu
/CEEd6NXEK1ZVN/7DFFVdiPjfPo0R6kphLxLIu/BnUEWvschv/gB876BAoGBAN/p
vUPMCq6TuLhEHhl5+ouvGVB6psotMjGOi/zFNEvxkvUe17LV/EmGN3gcTU8etpY9
8d7KMGd5mrFfF6w3cOZDx+zgXzHfH3KaHexFX5Uf03nYg7maYcJJJYKo8W7E63lm
QkfA1+bFk3sauDnxL6I8kxVBaX+vD79i72AvHKIHAoGBANylrB6PUMCcBbfPAK2j
W2sii1LvQOwV3CctaosbrLbzpP8K4KVg6vTf7TXj85igAMA8vKpRXzmp9TjNFm57
YR0abuVvUyGZikFIqCf07/YNth92mGo67Wzrqly8ukc+NcUXtlHgBjfaCIjzUak6
41hEPop7hzzK3a8JZttawTWBAoGBAJnteHPsCAfde2YJRdvjs+5kz/U4bAKesVvK
D8gT1ZDDoHAr5MKHQmlVADrs0eSrS3/bU7QBsObhfQukQITelBlnzT+1OxvwtBC3
jAyJ4FyGxX2E74EfdUb3u/anp1mru+j3+GaVVpbJikpdovoKKOHR3JcHIbxqH1xF
aiPoHznvAoGBAMTxikdrvaL6bzhIs4auk5MDXwZEVFGzAQophFrPaZGvVr6HFvY9
He/YXfk0Hhc1/MjYgosC3n/P/pgKR5a7iZgxKRexbjt0gfUV1Alpgle3LEwzGrEK
M0IvulZ1q8HXGz73UHfMxbWeZNIge9wqZT3cb9tx+vkZ7Gla0WjVQfR3
-----END RSA PRIVATE KEY-----
)EOF";


// Estructura para definir un puerto
struct Port {
    const char* name;
    float lat;
    float lng;
    float radius;
};

// Declaración de los puertos
const Port ports[] = {
    {"MFA", -45.394775, -72.712517, 10}, // Ejemplo: Santiago, Chile con un radio de 500 metros
    {"Port2", -34.6037, -58.3816, 1000}, // Ejemplo: Buenos Aires, Argentina con un radio de 1000 metros
    // Agrega más puertos según sea necesario
};

// OTA Configuration
const char* HOSTNAME = "TBEAM-GPS-Device";
const char* OTA_PASSWORD = "MFA_29052024"; // Contraseña segura

// Server URLs for HTTP requests
const char* SERVER_URL = "https://script.google.com/macros/s/AKfycbzIeSKDUaIP-3TIvQAVne6sLLvPvaarxSFdMtbFUn0PEQEQ3WhkqEJb390UxrjwL6DLiw/exec";
const char* PORT_URL = "https://script.google.com/macros/s/AKfycbyIGk-H4p1qjDOaGnln8muPg-Pjx9Jc6kGc1VJVPMdepe8R8hBcrvqDr44iYg92IU-k/exec";

// Definición de tokens de Blynk
#define TOKEN_SUNRISE "N-dURNFSPDyHSLugM16D63tLIsXMfeUX"
#define TOKEN_SUNSET "pkBo91IkD1Jed3pJ42LDsXG42J7JJFdl"
#define TOKEN_SUNSET_ICE "iNvkzmlZg_GflPlU6JY5_dHlKJ0hxsaW"
#define TOKEN_MOONSET "O7kzG4H8l9UiGRdUAv-W8xuYRezxkK_o"
#define TOKEN_FCOB "8IBclkkw_CF1MMwsl-eBvzSvOXljVjSV"
#define TOKEN_TRINIDAD "SSfjatyAs3rPdFGheg5GScCQajNinV0s"
#define TOKEN_DEFAULT "SSfjatyAs3rPdFGheg5GScCQajNinV11"

// Definición de direcciones MAC
#define MAC_SUNRISE "08:F9:E0:F5:95:9C"
#define MAC_SUNSET "34:98:7A:6C:65:78"
#define MAC_SUNSET_ICE "34:98:7A:6C:65:54"
#define MAC_MOONSET "34:98:7A:6C:67:80"
#define MAC_FCOB "XX:YY:YY:YY:YY:YY"
#define MAC_TRINIDAD "YY:YY:YY:YY:YY:YY"
#define MAC_DEFAULT "XX:YY:YY:YY:YY:YY"

// Battery and USB detection pin
#define BATTERY_PIN 35  // GPIO35 is used for battery level measurement
#define USB_POWER_PIN 34  // GPIO34 is used for USB power detection

// Función para configurar Blynk según la MAC del dispositivo
String deviceID = "NNNN-TBEAM";  // Valor por defecto

void configurarBlynk() {
    String macAddress = WiFi.macAddress();
    const char* token = TOKEN_DEFAULT;

    if (macAddress.equalsIgnoreCase(MAC_SUNRISE)) {
        token = TOKEN_SUNRISE;
        deviceID = "SNRS-TBEAM";

    } else if (macAddress.equalsIgnoreCase(MAC_SUNSET)) {
        token = TOKEN_SUNSET;
        deviceID = "SNST-TBEAM";
    } else if (macAddress.equalsIgnoreCase(MAC_SUNSET_ICE)) {
        token = TOKEN_SUNSET_ICE;
        deviceID = "SICE-TBEAM";
    } else if (macAddress.equalsIgnoreCase(MAC_MOONSET)) {
        token = TOKEN_MOONSET;
        deviceID = "MNST-TBEAM";
    } else if (macAddress.equalsIgnoreCase(MAC_FCOB)) {
        token = TOKEN_FCOB;
        deviceID = "FCOB-TBEAM";
    } else if (macAddress.equalsIgnoreCase(MAC_TRINIDAD)) {
        token = TOKEN_TRINIDAD;
        deviceID = "TRND-TBEAM";
    }

    Serial.print("Configuring Blynk with device: ");
    Serial.print(deviceID);
    Serial.print(", MAC: ");
    Serial.println(macAddress);

    Blynk.config(token);
    Blynk.connect();
}

#endif // CONFIG_H
