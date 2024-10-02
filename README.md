# TBEAM Rastreador GPS con Integración AWS IoT y Blynk

![Logo del Proyecto](https://yourprojectlogo.url/logo.png)

## Tabla de Contenidos

- [Introducción](#introducción)
- [Características](#características)
- [Requisitos de Hardware](#requisitos-de-hardware)
- [Requisitos de Software](#requisitos-de-software)
- [Instalación](#instalación)
- [Configuración](#configuración)
- [Uso](#uso)
- [Estructura del Proyecto](#estructura-del-proyecto)
- [Lógica de Operación](#lógica-de-operación)
- [Solución de Problemas](#solución-de-problemas)
- [Contribuciones](#contribuciones)
- [Licencia](#licencia)

## Introducción

Este proyecto implementa un rastreador GPS utilizando la placa de desarrollo **TTGO T-Beam** con ESP32, diseñado para monitoreo remoto y transmisión de datos a un servidor AWS IoT. Está optimizado para entornos de baja energía y puede operar de forma autónoma en condiciones desafiantes.

El dispositivo captura datos GPS, los procesa y los envía de forma segura a AWS IoT utilizando MQTT sobre TLS. También se integra con la plataforma Blynk, permitiendo el monitoreo y control en tiempo real a través de una aplicación móvil. El dispositivo es capaz de almacenar datos GPS localmente cuando la conectividad no está disponible y transmitirá los datos almacenados una vez que se restablezca la conexión.

## Características

- **Rastreo GPS en Tiempo Real**: Captura y procesa datos GPS utilizando la biblioteca TinyGPS++.
- **Integración con AWS IoT**: Transmite datos de forma segura a AWS IoT utilizando MQTT con cifrado TLS.
- **Almacenamiento Local de Datos**: Almacena datos GPS en SPIFFS cuando está fuera de línea y los transmite cuando se restablece la conectividad.
- **Integración con la App Blynk**: Permite a los usuarios monitorear el estado del dispositivo y los datos GPS en tiempo real a través de la aplicación móvil Blynk.
- **Gestión de Conectividad WiFi**: Se conecta automáticamente a redes WiFi disponibles y maneja reconexiones.
- **Actualizaciones OTA (Over-The-Air)**: Soporta actualizaciones remotas del firmware mediante ArduinoOTA.
- **Bajo Consumo de Energía**: Optimizado para operar con batería en entornos remotos.
- **Depuración y Registro Mejorados**: Registro mejorado para asistir en la depuración y monitoreo de la operación del dispositivo.
- **Interfaz de Comandos**: Acepta comandos a través del widget de terminal de Blynk para control y diagnóstico remotos.

## Requisitos de Hardware

- **Placa de Desarrollo TTGO T-Beam ESP32** con módulo GPS integrado.
- **Batería LiPo** (3.7V) para suministro de energía.
- **Cable Micro USB** para programación y carga.
- **Punto de Acceso WiFi** para conectividad de red (soporta múltiples redes).

## Requisitos de Software

- **Arduino IDE** (se recomienda la versión 1.8.13 o superior).
- **Soporte para placas ESP32** instalado mediante el Administrador de Placas del Arduino IDE.
- **Bibliotecas Requeridas** (instalar mediante el Administrador de Bibliotecas del Arduino IDE o descargarlas manualmente):
  - WiFiClientSecure
  - PubSubClient
  - TinyGPSPlus
  - BlynkSimpleEsp32
  - ArduinoJson
  - ESP32Time
  - ArduinoOTA
  - SPIFFS
  - Preferences
  - esp_task_wdt

## Instalación

1. **Clonar o Descargar el Repositorio**

   ```bash
   git clone https://github.com/tuusuario/tu-repo.git
   ```

2. **Instalar el Paquete de Placas ESP32**

   - Abrir el Arduino IDE.
   - Ir a `Archivo` > `Preferencias`.
   - En el campo "Gestor de URLs Adicionales de Tarjetas", agregar:

     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```

   - Ir a `Herramientas` > `Placa` > `Gestor de Tarjetas`, buscar "ESP32" e instalar el paquete.

3. **Instalar Bibliotecas Requeridas**

   Utilizar el Administrador de Bibliotecas del Arduino IDE para instalar todas las bibliotecas mencionadas en la sección de Requisitos de Software.

4. **Abrir el Proyecto**

   - Abrir `TBEAM_Optimizado.ino` en el Arduino IDE.

## Configuración

### 1. Actualizar Credenciales y Configuraciones

- **Credenciales WiFi**

  Edita `config.h` y reemplaza los marcadores de posición con tus SSIDs y contraseñas de WiFi:

  ```cpp
  const char* ssids[] = {
      "TU_SSID_1",
      "TU_SSID_2",
      // Añade más si es necesario
  };

  const char* passwords[] = {
      "TU_PASSWORD_1",
      "TU_PASSWORD_2",
      // Añade más si es necesario
  };
  ```

- **Credenciales de AWS IoT**

  Reemplaza los marcadores de posición en `config.h` con tu nombre de cosa (Thing Name) y endpoint de AWS IoT:

  ```cpp
  #define THING_NAME "TU_THING_NAME"
  #define AWS_IOT_ENDPOINT "TU_AWS_IOT_ENDPOINT"
  ```

- **Token de Blynk**

  Reemplaza el token de Blynk con el de tu proyecto:

  ```cpp
  #define TOKEN_SUNRISE "TU_BLYNK_TOKEN"
  ```

- **Certificados**

  En `certificates.h`, reemplaza los marcadores de posición con tus certificados reales:

  ```cpp
  const char* const cert_SNRS_TBEAM = R"EOF(
  -----BEGIN CERTIFICATE-----
  TU_CERTIFICADO_DISPOSITIVO
  -----END CERTIFICATE-----
  )EOF";

  const char* const key_SNRS_TBEAM = R"EOF(
  -----BEGIN RSA PRIVATE KEY-----
  TU_CLAVE_PRIVADA
  -----END RSA PRIVATE KEY-----
  )EOF";

  const char* const ca_SNRS_TBEAM = R"EOF(
  -----BEGIN CERTIFICATE-----
  TU_CERTIFICADO_CA_RAÍZ
  -----END CERTIFICATE-----
  )EOF";
  ```

- **Identificadores del Dispositivo**

  Actualiza los identificadores del dispositivo en `config.h` según sea necesario:

  ```cpp
  String deviceID = "TU_DEVICE_ID";
  String clientID = "TU_CLIENT_ID";
  ```

### 2. Configurar Actualizaciones OTA

- Establece el nombre de host y la contraseña OTA en `config.h`:

  ```cpp
  const char* HOSTNAME = "TBEAM-GPS-Device";
  const char* OTA_PASSWORD = "TU_CONTRASEÑA_OTA";
  ```

### 3. Ajustar Esquema de Partición (si es necesario)

- Asegúrate de que el tamaño de la partición SPIFFS sea adecuado.
- En el Arduino IDE, ve a `Herramientas` > `Esquema de Partición` y selecciona un esquema que proporcione suficiente espacio para SPIFFS, como "Default 4MB with spiffs".

## Uso

1. **Compilar y Cargar el Firmware**

   - Conecta tu placa TTGO T-Beam a tu computadora mediante USB.
   - Selecciona la placa correcta (`ESP32 Dev Module`) y el puerto en el Arduino IDE.
   - Haz clic en el botón `Cargar` para compilar y cargar el firmware.

2. **Monitorear la Salida Serial**

   - Abre el Monitor Serial a 115200 baudios para observar los registros y mensajes de depuración.

3. **Conectar a la App Blynk**

   - Descarga la aplicación Blynk en tu dispositivo móvil.
   - Crea un nuevo proyecto y añade los widgets necesarios (por ejemplo, Terminal, Visualización de Valores).
   - Utiliza el token que estableciste en el archivo `config.h`.

4. **Operación del Dispositivo**

   - El dispositivo intentará conectarse automáticamente a las redes WiFi configuradas.
   - Sincronizará el tiempo con un servidor NTP.
   - Los datos GPS se capturan cada minuto y se almacenan localmente si no hay conectividad.
   - Cuando está conectado, transmite los datos GPS almacenados a AWS IoT.

5. **Envío de Comandos a través de Blynk**

   - Utiliza el widget de Terminal en la aplicación Blynk para enviar comandos al dispositivo.
   - Comandos disponibles:

     ```
     reboot, status, restart_wifi, log on, log off, queue_status,
     sync_time, force_send, memory_status, gen_status, deep-sleep,
     clear, version, help
     ```

## Estructura del Proyecto

- **TBEAM_Optimizado.ino**: Archivo principal del firmware que contiene la lógica del dispositivo.
- **config.h**: Archivo de configuración para WiFi, AWS IoT, Blynk e identificadores del dispositivo.
- **certificates.h**: Contiene los certificados y claves del dispositivo para AWS IoT.
- **LoRaBoards.h**: Archivo de encabezado para configuraciones específicas de la placa (si es aplicable).

## Lógica de Operación

### Captura y Procesamiento de Datos

- Utiliza la biblioteca **TinyGPS++** para interpretar datos GPS del módulo GPS integrado.
- Procesa y formatea los datos GPS en JSON utilizando **ArduinoJson**.

### Gestión de Conectividad

- Administra las conexiones WiFi utilizando **WiFiMulti** para intentar conexiones a múltiples redes.
- Implementa lógica de reconexión no bloqueante para asegurar la captura continua de datos durante los intentos de reconexión.
- Se reconecta automáticamente al broker MQTT de AWS IoT utilizando **PubSubClient**.

### Transmisión de Datos

- Envía datos GPS a AWS IoT utilizando MQTT sobre TLS.
- Utiliza nivel de QoS 0 (como máximo una vez) para mensajes MQTT.
- Almacena datos GPS en **SPIFFS** cuando está fuera de línea y envía los datos cuando se restablece la conectividad.

### Almacenamiento Local con SPIFFS

- Los payloads se almacenan como archivos individuales en SPIFFS con nombres de archivo que indican su orden.
- Gestiona el espacio de almacenamiento eliminando los archivos más antiguos cuando el espacio es bajo.
- Asegura la integridad y el orden de los datos durante la transmisión.

### Integración con Blynk

- Permite el monitoreo en tiempo real del estado del dispositivo y datos GPS a través de la aplicación Blynk.
- Acepta comandos remotos mediante el widget de Terminal para diagnóstico y control.

### Gestión de Energía

- Verifica si el dispositivo está alimentado vía USB o batería.
- Ajusta los intervalos de muestreo de datos en función de la fuente de energía para conservar la vida de la batería.

### Actualizaciones OTA

- Soporta actualizaciones over-the-air mediante **ArduinoOTA**.
- Actualiza el firmware de forma segura sin acceso físico al dispositivo.

## Solución de Problemas

- **El Dispositivo No Se Conecta al WiFi**

  - Asegúrate de que los SSIDs y contraseñas en `config.h` sean correctos.
  - Verifica que las redes WiFi estén al alcance.

- **Falla en la Conexión a AWS IoT**

  - Verifica el endpoint de AWS IoT y el nombre de la cosa (Thing Name).
  - Asegúrate de que los certificados en `certificates.h` sean correctos y no hayan expirado.
  - Comprueba la conectividad a Internet.

- **No Se Capturan Datos GPS**

  - Asegúrate de que la antena GPS tenga una vista clara del cielo.
  - Permite unos minutos para que el módulo GPS adquiera una señal.

- **Las Actualizaciones OTA No Funcionan**

  - Verifica que el dispositivo esté conectado a la misma red que tu computadora.
  - Asegúrate de que el nombre de host y la contraseña OTA sean correctos.

- **No Se Envían los Payloads**

  - Verifica el estado de la conexión MQTT.
  - Utiliza los registros mejorados para identificar en qué punto falla el proceso de envío.
  - Asegúrate de que el dispositivo tenga suficiente memoria libre.

## Contribuciones

¡Las contribuciones son bienvenidas! Por favor, sigue estos pasos:

1. Haz un fork del repositorio.
2. Crea una nueva rama para tu característica o corrección de errores.
3. Realiza tus cambios con mensajes claros en los commits.
4. Envía un pull request detallando tus cambios.

Por favor, asegúrate de que tu código se adhiere al estilo existente y que todas las nuevas características están probadas.

## Licencia

Este proyecto está licenciado bajo la Licencia MIT. Consulta el archivo [LICENSE](LICENSE) para más detalles.

---

*Nota: Reemplaza los marcadores de posición como `TU_SSID_1`, `TU_AWS_IOT_ENDPOINT`, `TU_BLYNK_TOKEN` y el contenido de los certificados con tus credenciales y claves reales. No compartas información sensible públicamente.*

---