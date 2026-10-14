from django.db import models


class Lectura(models.Model):
    sensor_id = models.CharField(max_length=50)
    temp_1 = models.FloatField()
    hum_1 = models.FloatField()
    temp_2 = models.FloatField()
    hum_2 = models.FloatField()
    presencia_1 = models.BooleanField()
    presencia_2 = models.BooleanField()
    recibido_en = models.DateTimeField(auto_now_add=True, db_index=True)