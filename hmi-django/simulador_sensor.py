"""Simulador de la ESP32 del cuarto frío: publica por MQTT mensajes iguales a los
del firmware (formato en FORMATO_MQTT.md), para probar el HMI sin el hardware.

Uso (con el venv activado, desde la carpeta hmi-django):
    python simulador_sensor.py              # los 4 sensores funcionando
    python simulador_sensor.py --falla 3    # la Sección C "no responde" (ok_3 falso)
    python simulador_sensor.py --falla 1 4  # fallan las Secciones A y D
Se detiene con Ctrl+C.
"""
import argparse
import json
import random
import time

import paho.mqtt.client as mqtt

BROKER = "localhost"
PUERTO = 1883
TOPIC = "cuartofrio/datos"
SENSORES = (1, 2, 3, 4)      # Secciones A, B, C y D
CADA_SEG = 2                 # la ESP32 manda un mensaje cada ~2 s

parser = argparse.ArgumentParser(description="Simula la ESP32 del cuarto frío.")
parser.add_argument(
    "--falla", type=int, nargs="+", choices=SENSORES, default=[], metavar="N",
    help="número(s) de sensor (1 a 4) que se simulan desconectados (ok en falso)",
)
args = parser.parse_args()

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.connect(BROKER, PUERTO)
client.loop_start()

# Cada sección empieza en una temperatura un poco distinta
temperaturas = {n: 3.5 + 0.4 * n for n in SENSORES}

if args.falla:
    print("Sensores simulados como desconectados:", ", ".join(map(str, args.falla)))

try:
    while True:
        mensaje = {"sensor_id": "cuarto_frio_1"}
        for n in SENSORES:
            # Cada temperatura sube o baja un poquito en cada vuelta
            temperaturas[n] = max(0, min(10, temperaturas[n] + random.uniform(-0.3, 0.3)))
            if n in args.falla:
                # Igual que el firmware cuando un sensor no contesta: 0.0 y ok en falso
                mensaje[f"temp_{n}"] = 0.0
                mensaje[f"hum_{n}"] = 0.0
                mensaje[f"ok_{n}"] = False
            else:
                mensaje[f"temp_{n}"] = round(temperaturas[n], 1)
                mensaje[f"hum_{n}"] = round(random.uniform(60, 75), 1)
                mensaje[f"ok_{n}"] = True
        mensaje["presencia_1"] = random.random() < 0.1   # Secciones A y B
        mensaje["presencia_2"] = random.random() < 0.1   # Secciones C y D

        client.publish(TOPIC, json.dumps(mensaje))
        print("Enviado:", mensaje)
        time.sleep(CADA_SEG)
except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()
