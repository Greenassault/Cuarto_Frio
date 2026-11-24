"""Pruebas del HMI. Se corren con:  python manage.py test sensores
Usan una base de datos temporal: no tocan db.sqlite3."""
from datetime import timedelta

from django.test import TestCase
from django.utils import timezone

from .management.commands.escuchar_mqtt import mensaje_a_campos
from .models import DIAS_RETENCION, Lectura, borrar_lecturas_viejas

MENSAJE_4 = {
    "sensor_id": "cuarto_frio_1",
    "temp_1": 3.8, "hum_1": 68.2, "ok_1": True,
    "temp_2": 4.1, "hum_2": 67.5, "ok_2": True,
    "temp_3": 0.0, "hum_3": 0.0, "ok_3": False,
    "temp_4": 4.4, "hum_4": 70.1, "ok_4": True,
    "presencia_1": False, "presencia_2": True,
}


def crear(hace_seg=0, **cambios):
    """Guarda una lectura como si hubiera llegado hace `hace_seg` segundos."""
    lectura = Lectura.objects.create(**mensaje_a_campos({**MENSAJE_4, **cambios}))
    Lectura.objects.filter(pk=lectura.pk).update(
        recibido_en=timezone.now() - timedelta(seconds=hace_seg)
    )
    return lectura


class MensajeMqttTests(TestCase):
    def test_cuatro_sensores(self):
        c = mensaje_a_campos(MENSAJE_4)
        self.assertEqual((c["temp_4"], c["hum_4"], c["ok_4"]), (4.4, 70.1, True))
        self.assertEqual((c["presencia_1"], c["presencia_2"]), (False, True))

    def test_ok_falso_guarda_vacio(self):
        c = mensaje_a_campos(MENSAJE_4)
        self.assertEqual((c["temp_3"], c["hum_3"], c["ok_3"]), (None, None, False))

    def test_mensaje_viejo_de_dos_sensores(self):
        viejo = {k: v for k, v in MENSAJE_4.items() if k[-1] not in "34"}
        c = mensaje_a_campos(viejo)
        self.assertTrue(c["ok_1"])
        self.assertEqual((c["temp_4"], c["ok_4"]), (None, False))

    def test_ok_verdadero_sin_numero_no_cuenta(self):
        c = mensaje_a_campos({**MENSAJE_4, "temp_1": None, "hum_2": True})
        self.assertFalse(c["ok_1"])
        self.assertFalse(c["ok_2"])

    def test_falta_presencia_es_invalido(self):
        sin_presencia = {k: v for k, v in MENSAJE_4.items() if k != "presencia_2"}
        with self.assertRaises(KeyError):
            mensaje_a_campos(sin_presencia)


class ApiTests(TestCase):
    def test_stream_y_ultima_traen_los_4_sensores(self):
        crear()
        datos = self.client.get("/api/ultima/").json()
        for n in (1, 2, 3, 4):
            self.assertIn(f"temp_{n}", datos)
            self.assertIn(f"ok_{n}", datos)
        self.assertIsNone(datos["temp_3"])

    def test_grafica_ignora_lecturas_con_ok_falso(self):
        crear(hace_seg=5)
        crear(hace_seg=3, temp_1=99.0, ok_1=False)   # no debe mover el promedio
        crear(hace_seg=1, temp_1=4.2)
        puntos = self.client.get("/api/grafica/").json()["puntos"]
        n = [sum(p[1][i] for p in puntos) for i in range(4)]
        self.assertEqual(n, [2, 3, 0, 3])
        temps_1 = [p[2][0] for p in puntos if p[1][0]]
        self.assertTrue(all(t < 5 for t in temps_1))
        self.assertTrue(all(p[2][2] is None for p in puntos))   # sensor 3 nunca ok

    def test_resumen_ignora_lecturas_con_ok_falso(self):
        crear(hace_seg=60, temp_2=2.0)
        crear(hace_seg=30, temp_2=-30.0, ok_2=False)
        resumen = self.client.get("/api/historial/").json()["resumen"]
        self.assertEqual(resumen["temp_2_min"], 2.0)
        self.assertIsNone(resumen["temp_3_prom"])
        self.assertEqual(resumen["temp_4_max"], 4.4)


class RetencionTests(TestCase):
    def test_borra_solo_lo_de_mas_de_30_dias(self):
        dia = 24 * 3600
        crear(hace_seg=(DIAS_RETENCION + 1) * dia)
        nueva = crear(hace_seg=(DIAS_RETENCION - 1) * dia)
        self.assertEqual(borrar_lecturas_viejas(), 1)
        self.assertEqual(list(Lectura.objects.values_list("pk", flat=True)), [nueva.pk])


class PaginasTests(TestCase):
    def test_panel_tiene_4_secciones_y_2_presencias(self):
        html = self.client.get("/").content.decode()
        for n in (1, 2, 3, 4):
            self.assertIn(f'id="estado-sensor_{n}"', html)
        self.assertIn("Cuarto frío 1 - Sección D", html)
        self.assertIn('id="presencia_2"', html)
        # La presencia 1 va con A y B; la 2 con C y D
        self.assertLess(html.index("Sección B"), html.index('id="presencia_1"'))
        self.assertLess(html.index('id="presencia_1"'), html.index("Sección C"))

    def test_base_de_datos_muestra_guiones_si_no_responde(self):
        crear()
        html = self.client.get("/datos/").content.decode()
        self.assertIn("Cuarto frío 1 - Sección C", html)
        self.assertIn(">--</td>", html)
        self.assertIn(">4.4</td>", html)

    def test_csv_con_columnas_nuevas(self):
        crear()
        respuesta = self.client.get("/datos/csv/")
        lineas = b"".join(respuesta.streaming_content).decode("utf-8-sig").splitlines()
        encabezados = lineas[0].split(",")
        self.assertEqual(len(encabezados), 1 + 4 * 2 + 2)
        self.assertEqual(encabezados[-1], "Presencia - Secciones C y D")
        fila = lineas[1].split(",")
        self.assertEqual(fila[5:7], ["", ""])          # Sección C sin respuesta: vacío
        self.assertEqual(fila[7:11], ["4.4", "70.1", "No", "Sí"])

    def test_paginas_de_graficas_cargan(self):
        for url in ("/grafica/", "/historial/"):
            self.assertEqual(self.client.get(url).status_code, 200)
