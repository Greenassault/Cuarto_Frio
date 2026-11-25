import json
import time

import paho.mqtt.client as mqtt
from django.core.management.base import BaseCommand

from sensores.models import DIAS_RETENCION, SENSORES, Lectura, borrar_lecturas_viejas

BROKER = "localhost"
PUERTO = 1883
TOPIC = "cuartofrio/datos"

# Cada cuánto se borran las lecturas con más de DIAS_RETENCION días
LIMPIEZA_CADA_SEG = 3600


def numero(valor):
    """El valor si es un número; None si falta o no es número (true/false no cuentan)."""
    if isinstance(valor, (int, float)) and not isinstance(valor, bool):
        return float(valor)
    return None


def mensaje_a_campos(datos):
    """Convierte el JSON de la ESP32 (ver FORMATO_MQTT.md) en los campos de Lectura.

    Un sensor solo cuenta como bien (ok) si el mensaje dice ok_N: true y trae
    temp_N y hum_N como números. Si no, sus valores se guardan vacíos.
    Lanza KeyError si falta sensor_id, presencia_1 o presencia_2.
    """
    campos = {
        "sensor_id": str(datos["sensor_id"]),
        "presencia_1": datos["presencia_1"] is True,
        "presencia_2": datos["presencia_2"] is True,
    }
    for n in SENSORES:
        temp = numero(datos.get(f"temp_{n}"))
        hum = numero(datos.get(f"hum_{n}"))
        ok = datos.get(f"ok_{n}") is True and temp is not None and hum is not None
        campos[f"ok_{n}"] = ok
        campos[f"temp_{n}"] = temp if ok else None
        campos[f"hum_{n}"] = hum if ok else None
    return campos


class Command(BaseCommand):
    help = (
        "Escucha los mensajes MQTT del cuarto frío y los guarda en la base de datos. "
        f"Cada hora borra las lecturas con más de {DIAS_RETENCION} días."
    )

    def handle(self, *args, **options):
        self.ultima_limpieza = None
        self.limpiar_si_toca()
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        client.on_connect = self.on_connect
        client.on_message = self.on_message
        client.connect(BROKER, PUERTO)
        self.stdout.write("Escuchando mensajes MQTT... (Ctrl+C para detener)")
        client.loop_forever()

    def limpiar_si_toca(self):
        ahora = time.monotonic()
        if self.ultima_limpieza is not None and ahora - self.ultima_limpieza < LIMPIEZA_CADA_SEG:
            return
        self.ultima_limpieza = ahora
        borradas = borrar_lecturas_viejas()
        if borradas:
            self.stdout.write(f"Se borraron {borradas} lecturas con más de {DIAS_RETENCION} días")

    def on_connect(self, client, userdata, flags, reason_code, properties):
        client.subscribe(TOPIC)
        self.stdout.write(f"Suscrito a {TOPIC}")

    def on_message(self, client, userdata, msg):
        try:
            datos = json.loads(msg.payload.decode())
            Lectura.objects.create(**mensaje_a_campos(datos))
            self.stdout.write(f"Guardado: {datos}")
        except (UnicodeDecodeError, json.JSONDecodeError, KeyError, TypeError, ValueError) as error:
            self.stderr.write(f"Mensaje inválido ignorado: {error!r}")
        self.limpiar_si_toca()
