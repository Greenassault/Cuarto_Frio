from datetime import timedelta

from django.db import models
from django.utils import timezone

# Sensores de temperatura y humedad (XY-MD03) en el cuarto frío: 1 a 4.
# El formato del mensaje está documentado en FORMATO_MQTT.md (raíz del repo).
SENSORES = (1, 2, 3, 4)

# Las lecturas con más de estos días se borran solas (ver borrar_lecturas_viejas).
DIAS_RETENCION = 30


class Lectura(models.Model):
    """Un mensaje de la ESP32: 4 sensores de temperatura/humedad y 2 de presencia.

    Si ok_N es falso, el sensor N no respondió: temp_N y hum_N quedan vacíos (None)
    y esa lectura no se usa en gráficas ni promedios. Las lecturas de cuando había
    solo 2 sensores tienen los sensores 3 y 4 vacíos y en ok falso.
    """

    sensor_id = models.CharField(max_length=50)
    temp_1 = models.FloatField(null=True, blank=True)
    hum_1 = models.FloatField(null=True, blank=True)
    ok_1 = models.BooleanField(default=False)
    temp_2 = models.FloatField(null=True, blank=True)
    hum_2 = models.FloatField(null=True, blank=True)
    ok_2 = models.BooleanField(default=False)
    temp_3 = models.FloatField(null=True, blank=True)
    hum_3 = models.FloatField(null=True, blank=True)
    ok_3 = models.BooleanField(default=False)
    temp_4 = models.FloatField(null=True, blank=True)
    hum_4 = models.FloatField(null=True, blank=True)
    ok_4 = models.BooleanField(default=False)
    presencia_1 = models.BooleanField()
    presencia_2 = models.BooleanField()
    recibido_en = models.DateTimeField(auto_now_add=True, db_index=True)


def borrar_lecturas_viejas():
    """Borra las lecturas con más de DIAS_RETENCION días. Devuelve cuántas borró."""
    limite = timezone.now() - timedelta(days=DIAS_RETENCION)
    borradas, _ = Lectura.objects.filter(recibido_en__lt=limite).delete()
    return borradas
