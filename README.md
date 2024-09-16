# Proyecto de Monitoreo GPS con ESP32

## Descripción

Este proyecto utiliza un dispositivo basado en ESP32 para el monitoreo remoto de datos GPS y su transmisión a un servidor AWS IoT. El dispositivo emplea diversas tecnologías y bibliotecas como FreeRTOS, Blynk, LoRa, MQTT, OTA (Over-the-Air updates) y técnicas de manejo de energía para optimizar el funcionamiento en condiciones remotas.

## Objetivos del Proyecto

- **Monitoreo en Tiempo Real**: Capturar y enviar datos de GPS a un servidor remoto de AWS IoT.
- **Optimización del Consumo de Energía**: Utilizar modos de bajo consumo y técnicas de manejo de energía para maximizar la duración de la batería.
- **Gestión de Conectividad**: Reconectar automáticamente a WiFi y AWS IoT en caso de desconexión, y almacenar los datos localmente para su posterior envío.
- **Actualizaciones Seguras OTA**: Permitir actualizaciones de firmware OTA de manera segura.
- **Interfaz de Usuario con Blynk**: Proporcionar una interfaz de usuario fácil de usar para monitorear el estado del dispositivo y enviar comandos.

## Requerimientos de Hardware

- **ESP32**: Microcontrolador principal.
- **Módulo GPS**: Para capturar datos de ubicación.
- **Módulo LoRa**: Para comunicaciones de largo alcance (opcional).
- **Módulo de energía**: Para manejar la carga de batería y optimizar el consumo de energía.
- **Starlink o Conexión WiFi**: Para conectividad en áreas remotas.

## Requerimientos de Software

- **Arduino IDE o PlatformIO**: Entorno de desarrollo para el ESP32.
- **Bibliotecas Arduino**:
  - `WiFi.h`, `WiFiMulti.h`
  - `PubSubClient.h` (para MQTT)
  - `BlynkSimpleEsp32.h`
  - `ArduinoJson.h`
  - `TinyGPS++.h`
  - `FreeRTOS.h` (incluida en el ESP32)
  - `ArduinoOTA.h` (para actualizaciones OTA)
  - `Preferences.h` (para almacenamiento no volátil)
  - Otras bibliotecas relacionadas con hardware específico (LoRa, GPS).

## Archivos Principales

- `TBEAM_03_09_hilos.ino`: Código principal que configura las tareas de FreeRTOS, maneja la conectividad y la recolección de datos GPS.
- `config.h`: Archivo de configuración que contiene definiciones y configuraciones globales.
- `LoRaBoards.cpp` y `LoRaBoards.h`: Implementaciones relacionadas con el uso de LoRa para comunicaciones inalámbricas.
- `utilities.h`: Funciones auxiliares para el manejo de logs y otras operaciones.

## Instalación

1. **Clona el repositorio**:

   ```bash
   git clone https://github.com/tuusuario/tu-repositorio.git
   cd tu-repositorio
   ```

2. **Configura el Proyecto**:

   - Abre el archivo `config.h` y configura tus credenciales WiFi, el token de Blynk, los certificados de AWS IoT, etc.
   - Asegúrate de que las bibliotecas necesarias estén instaladas en tu entorno de desarrollo (Arduino IDE o PlatformIO).

3. **Compila y Carga el Código**:

   - Conecta tu ESP32 a tu computadora.
   - Utiliza el Arduino IDE o PlatformIO para compilar y cargar el código al dispositivo.

## Uso

- **Interfaz Blynk**: Abre la aplicación Blynk en tu dispositivo móvil y conecta con el dispositivo para monitorear datos en tiempo real.
- **Supervisión de Conectividad**: El dispositivo intentará reconectarse automáticamente a WiFi y AWS IoT en caso de desconexión.
- **Actualizaciones OTA**: El dispositivo está configurado para recibir actualizaciones de firmware OTA de manera segura.

## Estructura del Proyecto

```bash
├── TBEAM_03_09_hilos.ino      # Código principal del proyecto
├── config.h                   # Archivo de configuración global
├── LoRaBoards.cpp             # Implementación de funciones relacionadas con LoRa
├── LoRaBoards.h               # Declaración de funciones relacionadas con LoRa
├── utilities.h                # Funciones de utilidad
└── certificates.h             # Certificados para la conexión segura con AWS IoT
```

## Contribuciones

Las contribuciones son bienvenidas. Por favor, sigue los siguientes pasos:

1. **Haz un fork del proyecto y crea tu rama de características** (`feature/nueva-caracteristica`).
2. **Haz commit de tus cambios** (`git commit -am 'Añade una nueva característica'`).
3. **Haz push a la rama** (`git push origin feature/nueva-caracteristica`).
4. **Crea un Pull Request**.

## Licencia

Este proyecto está bajo la Licencia MIT. Para más detalles, revisa el archivo `LICENSE`.

## Contacto

Si tienes alguna pregunta o necesitas soporte, por favor contacta a [proyectos@seawatch.cl](mailto:proyectos@seawatch.cl).

## Notas Adicionales

- Este dispositivo está diseñado para operar en condiciones remotas utilizando Starlink para conectividad donde otras redes no están disponibles.
- Asegúrate de que el dispositivo esté configurado correctamente para operar 24/7 en un entorno con conectividad limitada y alta eficiencia energética.
