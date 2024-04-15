import network
import urequests
from machine import UART, Pin
import time
import ujson

# Configuración del UART para GPS
uart = UART(2, baudrate=9600, tx=17, rx=16)

# Conexión Wi-Fi
ssid = 'Tu_SSID'
password = 'Tu_Contrasena'
station = network.WLAN(network.STA_IF)
station.active(True)
station.connect(ssid, password)

while not station.isconnected():
    time.sleep(0.1)
print('Conectado a Wi-Fi')

def read_gps():
    data = uart.readline()
    if data:
        return data.decode('utf-8')
    return None

def parse_gps(data):
    # Implementar la lógica de parseo del NMEA o datos del GPS aquí
    return data  # Retornar datos parseados como diccionario o similar

def send_data(url, data):
    headers = {'content-type': 'application/json'}
    response = urequests.post(url, data=ujson.dumps(data), headers=headers)
    print(response.text)

# URL del servidor para enviar datos
url = 'http://tu-servidor.com/api/datos'

while True:
    gps_data = read_gps()
    if gps_data:
        parsed_data = parse_gps(gps_data)
        send_data(url, parsed_data)
    time.sleep(10)
