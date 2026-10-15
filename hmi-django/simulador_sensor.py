import json
import random
import time

import paho.mqtt.client as mqtt

BROKER = "localhost"
PUERTO = 1883
TOPIC = "cuartofrio/datos"

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.connect(BROKER, PUERTO)
client.loop_start()

temp_1 = 4.0
temp_2 = 4.5

try:
    while True:
        # Cada temperatura sube o baja un poquito en cada vuelta
        temp_1 += random.uniform(-0.3, 0.3)
        temp_1 = max(0, min(10, temp_1))
        temp_2 += random.uniform(-0.3, 0.3)
        temp_2 = max(0, min(10, temp_2))

        mensaje = {
            "sensor_id": "cuarto_frio_1",
            "temp_1": round(temp_1, 1),
            "hum_1": round(random.uniform(60, 75), 1),
            "temp_2": round(temp_2, 1),
            "hum_2": round(random.uniform(60, 75), 1),
            "presencia_1": random.random() < 0.1,
            "presencia_2": random.random() < 0.1,
        }

        client.publish(TOPIC, json.dumps(mensaje))
        print("Enviado:", mensaje)
        time.sleep(2)
except KeyboardInterrupt:
    client.loop_stop()
    client.disconnect()