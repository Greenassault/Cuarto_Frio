import json

import paho.mqtt.client as mqtt
from django.core.management.base import BaseCommand

from sensores.models import Lectura

BROKER = "localhost"
PUERTO = 1883
TOPIC = "cuartofrio/datos"


class Command(BaseCommand):
    help = "Escucha los mensajes MQTT del cuarto frío y los guarda en la base de datos"

    def handle(self, *args, **options):
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        client.on_connect = self.on_connect
        client.on_message = self.on_message
        client.connect(BROKER, PUERTO)
        self.stdout.write("Escuchando mensajes MQTT... (Ctrl+C para detener)")
        client.loop_forever()

    def on_connect(self, client, userdata, flags, reason_code, properties):
        client.subscribe(TOPIC)
        self.stdout.write(f"Suscrito a {TOPIC}")

    def on_message(self, client, userdata, msg):
        try:
            datos = json.loads(msg.payload.decode())
            Lectura.objects.create(
                sensor_id=datos["sensor_id"],
                temp_1=datos["temp_1"],
                hum_1=datos["hum_1"],
                temp_2=datos["temp_2"],
                hum_2=datos["hum_2"],
                presencia_1=datos["presencia_1"],
                presencia_2=datos["presencia_2"],
            )
            self.stdout.write(f"Guardado: {datos}")
        except (json.JSONDecodeError, KeyError, ValueError) as error:
            self.stderr.write(f"Mensaje inválido ignorado: {error}")