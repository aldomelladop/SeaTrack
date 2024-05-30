# SeaTrack

## Descripción del Proyecto
Este proyecto desarrolla un sistema de rastreo y monitoreo para lanchas utilizando la placa LilyGO T-Beam v1.1, conocida por su integración de GPS, LoRa y WiFi, lo que la hace ideal para aplicaciones de monitoreo remoto en ambientes marítimos. Este sistema no solo proporciona localización precisa mediante GPS y un filtro de Kalman para minimizar el ruido en los datos de localización, sino que también está diseñado para integrar futuros módulos de monitoreo de parámetros críticos en lanchas, como impactos, niveles de combustible y otros sensores ambientales y mecánicos.

## Características Principales
- **Filtro de Kalman**: Optimiza la precisión de los datos de localización GPS.
- **Conectividad WiFi y LoRa**: Permite la transmisión de datos a larga distancia y a través de internet.
- **Sincronización de Hora NTP**: Garantiza que todos los eventos registrados tengan una estampa de tiempo precisa, esencial para las bitácoras y el seguimiento de rutas.
- **Actualizaciones OTA (Over-The-Air)**: Facilita la actualización y el mantenimiento del software a distancia.

## Especificaciones de Hardware
- **Placa LilyGO T-Beam v1.1**: Equipada con ESP32, GPS NEO-6M y módulo LoRa, ideal para seguimiento y comunicación en áreas remotas.
- **Sensores Adicionales**:
  - **Sensores de Impacto**: Para detectar colisiones o condiciones de navegación extremas.
  - **Medidores de Nivel de Combustible**: Para gestionar el consumo y la autonomía de la embarcación.
  - **Sensores Ambientales**: Como termómetros y barómetros, para monitorear las condiciones del entorno.

## Configuración y Uso
1. **Montaje del Hardware**: Conectar todos los sensores necesarios a la placa LilyGO T-Beam según las especificaciones de cada componente.
2. **Configuración de la Red**: Ajustar los parámetros de WiFi y LoRa en el archivo `config.h` para garantizar una conectividad óptima.
3. **Carga del Software**: Utilizar el IDE de Arduino para programar la placa, asegurándose de seleccionar la configuración adecuada para la T-Beam v1.1.
4. **Configuración de Tiempos de Suspensión**: Ajustar los tiempos de suspensión para que el dispositivo entre en modo de suspensión después de transmitir datos exitosamente. Actualmente, el tiempo de espera es de 10 segundos y el intervalo de suspensión es de 1.5 minutos si está conectado a USB y 1.75 minutos si no está conectado a USB.

### Configuración de `config.h`
```cpp
const char* ssids[] = {"WiFi_SSID1", "WiFi_SSID2"};
const char* passwords[] = {"WiFi_PASSWORD1", "WiFi_PASSWORD2"};
const char* SERVER_URL = "https://example.com/path_to_your_script";
const char* HOSTNAME = "TBEAM-GPS-Device";
const char* OTA_PASSWORD = "your_ota_password";
char auth[] = "your_blynk_auth_token";
```

## Proyectos Futuros y Ampliación
* **Monitoreo Integral de Embarcaciones**: Incluir sensores para el seguimiento de la salud estructural del casco, sistemas eléctricos y eficiencia del motor.
* **Bitácora Automática**: Desarrollar un sistema que genere registros automáticos de cada viaje, incluyendo rutas, condiciones operativas y eventos notables.
* **Sistema de Alertas Proactivo**: Implementar un sistema de alertas que notifique a los operadores y a las bases en tierra sobre condiciones críticas o mantenimiento necesario.

## Otras Aplicaciones de Monitoreo
* **Seguridad Personal en Navegación**: Dispositivos personales que alerten sobre hombre al agua o desviaciones de una ruta planificada.
* **Investigación Oceanográfica**: Uso de flotas de drones marinos equipados con similares sistemas de monitoreo para recopilación de datos marinos a gran escala.
* **Turismo y Recreación**: Ofrecer a los operadores turísticos herramientas para el seguimiento y la gestión de sus flotas en tiempo real, mejorando la seguridad y la experiencia del usuario.

## Contribuciones
Este proyecto está abierto a colaboraciones. Se invita a los desarrolladores e investigadores interesados en tecnologías marinas a contribuir con código, ideas y pruebas de campo.

## Licencia
Distribuido bajo la Licencia MIT, permitiendo el uso libre y la integración en aplicaciones comerciales sin restricciones.
