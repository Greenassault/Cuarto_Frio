import json
import time
from datetime import datetime, timedelta

from django.db.models import Avg, Max, Min
from django.http import JsonResponse, StreamingHttpResponse
from django.shortcuts import render
from django.utils import timezone

from .models import Lectura

def lectura_a_dict(lectura):
    return {
        "sensor_id": lectura.sensor_id,
        "temp_1": lectura.temp_1,
        "hum_1": lectura.hum_1,
        "temp_2": lectura.temp_2,
        "hum_2": lectura.hum_2,
        "presencia_1": lectura.presencia_1,
        "presencia_2": lectura.presencia_2,
        "recibido_en": lectura.recibido_en.isoformat(),
    }


def ultima_lectura(request):
    lectura = Lectura.objects.order_by("-recibido_en").first()
    if lectura is None:
        return JsonResponse({"error": "sin datos"}, status=404)
    return JsonResponse(lectura_a_dict(lectura))


def stream(request):
    def eventos():
        ultimo_id = None
        while True:
            lectura = Lectura.objects.order_by("-recibido_en").first()
            if lectura is not None and lectura.id != ultimo_id:
                ultimo_id = lectura.id
                yield f"data: {json.dumps(lectura_a_dict(lectura))}\n\n"
            else:
                yield ": keepalive\n\n"
            time.sleep(1)

    respuesta = StreamingHttpResponse(eventos(), content_type="text/event-stream")
    respuesta["Cache-Control"] = "no-cache"
    return respuesta


def panel(request):
    return render(request, "sensores/panel.html")

MAX_PUNTOS = 1500
MAX_TABLA = 100


def parsear_fecha(texto):
    if not texto:
        return None
    fecha = datetime.fromisoformat(texto.replace("Z", "+00:00"))
    if timezone.is_naive(fecha):
        fecha = timezone.make_aware(fecha)
    return fecha


def historial(request):
    return render(request, "sensores/historial.html")


def api_historial(request):
    try:
        ahora = timezone.now()
        hasta = parsear_fecha(request.GET.get("hasta")) or ahora
        desde = parsear_fecha(request.GET.get("desde")) or hasta - timedelta(hours=24)
        umbral = int(request.GET.get("umbral", 300))
    except ValueError:
        return JsonResponse({"error": "parámetros inválidos"}, status=400)

    if desde >= hasta or umbral < 1:
        return JsonResponse({"error": "rango inválido"}, status=400)

    filas = Lectura.objects.filter(recibido_en__gte=desde, recibido_en__lte=hasta)
    total = filas.count()
    paso = max(1, total // MAX_PUNTOS)

    # Se recorren todas las lecturas para detectar huecos,
    # pero solo se guardan algunas para la gráfica.
    puntos = []
    huecos = []
    anterior = None
    campos = ("recibido_en", "temp_1", "hum_1", "temp_2", "hum_2")
    consulta = filas.order_by("recibido_en").values_list(*campos)
    for i, fila in enumerate(consulta.iterator()):
        momento = fila[0]
        if anterior is not None:
            segundos = (momento - anterior).total_seconds()
            if segundos > umbral:
                huecos.append({
                    "desde": anterior.isoformat(),
                    "hasta": momento.isoformat(),
                    "segundos": int(segundos),
                })
        anterior = momento
        if i % paso == 0:
            puntos.append([momento.isoformat(), *fila[1:]])

    # Hueco abierto: la última lectura es vieja y no ha llegado nada nuevo
    fin = min(hasta, ahora)
    if anterior is not None:
        segundos = (fin - anterior).total_seconds()
        if segundos > umbral:
            huecos.append({
                "desde": anterior.isoformat(),
                "hasta": fin.isoformat(),
                "segundos": int(segundos),
            })

    resumen = filas.aggregate(
        temp_1_min=Min("temp_1"), temp_1_max=Max("temp_1"), temp_1_prom=Avg("temp_1"),
        temp_2_min=Min("temp_2"), temp_2_max=Max("temp_2"), temp_2_prom=Avg("temp_2"),
    )

    ultimas = [
        lectura_a_dict(lectura)
        for lectura in filas.order_by("-recibido_en")[:MAX_TABLA]
    ]

    return JsonResponse({
        "desde": desde.isoformat(),
        "hasta": hasta.isoformat(),
        "total": total,
        "paso": paso,
        "puntos": puntos,
        "huecos": huecos,
        "resumen": resumen,
        "ultimas": ultimas,
    })