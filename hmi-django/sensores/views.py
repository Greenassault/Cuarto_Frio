import csv
import json
import time
from datetime import datetime, timedelta, timezone as zona_utc

from django.core.paginator import Paginator
from django.db.models import Avg, Max, Min, Q
from django.http import JsonResponse, StreamingHttpResponse
from django.shortcuts import render
from django.utils import timezone
from django.views.decorators.http import require_GET

from .models import SENSORES, Lectura

# Nombres que ve el usuario. La clave es el número del sensor de temperatura
# (temp_1, hum_1, ok_1 -> 1). Para cambiar un nombre en toda la interfaz, editar aquí.
# Qué dirección Modbus tiene cada número: ver FORMATO_MQTT.md.
NOMBRE_CUARTO = "Cuarto frío 1"
NOMBRES_SECCIONES = {
    1: "Sección A",
    2: "Sección B",
    3: "Sección C",
    4: "Sección D",
}
NOMBRES_SENSORES = {
    numero: f"{NOMBRE_CUARTO} - {seccion}"
    for numero, seccion in NOMBRES_SECCIONES.items()
}


# Nombres de los botones del panel; también son el título de cada página destino.
NOMBRES_BOTONES = {
    "estadisticas": "Ver estadísticas",
    "base_datos": "Ver base de datos",
}


def contexto_nombres():
    return {
        "nombres": NOMBRES_SENSORES,
        "secciones": NOMBRES_SECCIONES,
        "botones": NOMBRES_BOTONES,
    }

def lectura_a_dict(lectura):
    """Lectura -> diccionario para el stream en vivo y la API (mismos nombres que
    el mensaje MQTT, ver FORMATO_MQTT.md). Si ok_N es falso, temp_N y hum_N son None."""
    datos = {"sensor_id": lectura.sensor_id}
    for n in SENSORES:
        datos[f"temp_{n}"] = getattr(lectura, f"temp_{n}")
        datos[f"hum_{n}"] = getattr(lectura, f"hum_{n}")
        datos[f"ok_{n}"] = getattr(lectura, f"ok_{n}")
    datos["presencia_1"] = lectura.presencia_1
    datos["presencia_2"] = lectura.presencia_2
    datos["recibido_en"] = lectura.recibido_en.isoformat()
    return datos


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
    return render(request, "sensores/panel.html", contexto_nombres())

MAX_TABLA = 100

# Promedios para las gráficas: se dibuja un punto por intervalo en vez de una por lectura.
# El intervalo es el más corto de esta lista (en segundos: 1, 2, 5, 10, 15 y 30 min;
# 1, 2, 4, 6 y 12 h; 1 día) que deja como máximo MAX_PUNTOS_GRAFICA puntos.
# Rangos típicos: 1 h -> 1 min, 24 h -> 5 min, 7 días -> 30 min, 30 días -> 2 h.
INTERVALOS_GRAFICA = (60, 120, 300, 600, 900, 1800, 3600, 7200, 14400, 21600, 43200, 86400)
MAX_PUNTOS_GRAFICA = 400


def intervalo_grafica(duracion_seg):
    for intervalo in INTERVALOS_GRAFICA:
        if duracion_seg / intervalo <= MAX_PUNTOS_GRAFICA:
            return intervalo
    return INTERVALOS_GRAFICA[-1]


# Campos que recibe Promediador.sumar, en este orden: ok_1, temp_1, hum_1, ok_2, ...
CAMPOS_GRAFICA = tuple(f"{campo}_{n}" for n in SENSORES for campo in ("ok", "temp", "hum"))


class Promediador:
    """Agrupa lecturas por intervalo y promedia cada sensor por separado.

    Solo sirve para dibujar: no modifica ni guarda nada. De cada sensor solo se
    usan las lecturas con ok verdadero. Cada punto sale como
    [inicio del intervalo, [n_1..n_4], [temp_1..temp_4], [hum_1..hum_4]]
    donde n_N es cuántas lecturas buenas tuvo el sensor N en ese intervalo (para
    que el navegador pueda seguir sumando lecturas nuevas en vivo). Si n_N es 0,
    el promedio de ese sensor es None y la gráfica deja un hueco.
    """

    def __init__(self, intervalo):
        self.intervalo = intervalo
        self.grupos = {}

    def sumar(self, momento, valores):
        segundos = int(momento.timestamp())
        inicio = segundos - segundos % self.intervalo
        # Por sensor: [lecturas buenas, suma de temperatura, suma de humedad]
        grupo = self.grupos.setdefault(inicio, [[0, 0.0, 0.0] for _ in SENSORES])
        for i, sensor in enumerate(grupo):
            ok, temp, hum = valores[3 * i:3 * i + 3]
            if ok and temp is not None and hum is not None:
                sensor[0] += 1
                sensor[1] += temp
                sensor[2] += hum

    def puntos(self):
        def promedio(suma, n):
            return round(suma / n, 2) if n else None

        resultado = []
        for inicio in sorted(self.grupos):
            grupo = self.grupos[inicio]
            resultado.append([
                datetime.fromtimestamp(inicio, tz=zona_utc.utc).isoformat(),
                [n for n, _, _ in grupo],
                [promedio(temp, n) for n, temp, _ in grupo],
                [promedio(hum, n) for n, _, hum in grupo],
            ])
        return resultado


def parsear_fecha(texto):
    if not texto:
        return None
    fecha = datetime.fromisoformat(texto.replace("Z", "+00:00"))
    if timezone.is_naive(fecha):
        fecha = timezone.make_aware(fecha)
    return fecha


def historial(request):
    return render(request, "sensores/historial.html", contexto_nombres())


def grafica(request):
    return render(request, "sensores/grafica.html", contexto_nombres())


VENTANA_GRAFICA = timedelta(hours=1)


@require_GET
def api_grafica(request):
    """Última hora para la gráfica en vivo, promediada (ver Promediador).

    "ultima" es la hora de la última lectura incluida: el navegador sigue en vivo
    solo con lecturas posteriores, para no contar ninguna dos veces.
    """
    intervalo = intervalo_grafica(VENTANA_GRAFICA.total_seconds())
    desde = (timezone.now() - VENTANA_GRAFICA).replace(second=0, microsecond=0)
    consulta = (
        Lectura.objects.filter(recibido_en__gte=desde)
        .order_by("recibido_en")
        .values_list("recibido_en", *CAMPOS_GRAFICA)
    )
    promediador = Promediador(intervalo)
    ultima = None
    for momento, *valores in consulta.iterator():
        promediador.sumar(momento, valores)
        ultima = momento
    return JsonResponse({
        "intervalo": intervalo,
        "ultima": ultima.isoformat() if ultima else None,
        "puntos": promediador.puntos(),
    })


LECTURAS_POR_PAGINA = 50


def lecturas_filtradas(request):
    """Lecturas más recientes primero, filtradas por ?desde= y ?hasta= (opcionales).

    Devuelve (consulta, mensaje_de_error); el error es None si todo está bien.
    """
    try:
        desde = parsear_fecha(request.GET.get("desde"))
        hasta = parsear_fecha(request.GET.get("hasta"))
    except ValueError:
        return Lectura.objects.none(), "Las fechas no son válidas."
    if desde and hasta and desde >= hasta:
        return Lectura.objects.none(), "Revisa que 'Desde' sea anterior a 'Hasta'."
    filas = Lectura.objects.order_by("-recibido_en")
    if desde:
        filas = filas.filter(recibido_en__gte=desde)
    if hasta:
        filas = filas.filter(recibido_en__lte=hasta)
    return filas, None


@require_GET
def datos(request):
    filas, error = lecturas_filtradas(request)
    pagina = Paginator(filas, LECTURAS_POR_PAGINA).get_page(request.GET.get("pagina"))
    filtro = request.GET.copy()
    filtro.pop("pagina", None)
    return render(request, "sensores/datos.html", {
        **contexto_nombres(),
        "pagina": pagina,
        "error": error,
        "filtro": filtro.urlencode(),
        "desde": request.GET.get("desde", ""),
        "hasta": request.GET.get("hasta", ""),
    })


class _Escritura:
    """Permite que csv.writer entregue cada fila para enviarla sin armar todo el archivo."""

    def write(self, texto):
        return texto


@require_GET
def datos_csv(request):
    filas, error = lecturas_filtradas(request)
    if error:
        return JsonResponse({"error": error}, status=400)

    # El navegador manda su desfase con la hora UTC (en minutos) para escribir horas locales
    try:
        desfase = max(-840, min(840, int(request.GET.get("tz", 0))))
    except ValueError:
        desfase = 0

    encabezados = ["Fecha y hora"]
    for nombre in NOMBRES_SENSORES.values():
        encabezados += [
            f"{nombre} - Temperatura (°C)",
            f"{nombre} - Humedad (%)",
            f"{nombre} - Presencia",
        ]

    def si_no(valor):
        return "Sí" if valor else "No"

    def generar():
        escritor = csv.writer(_Escritura())
        yield "﻿"  # marca para que Excel lea bien los acentos
        yield escritor.writerow(encabezados)
        consulta = filas.values_list(
            "recibido_en", "temp_1", "hum_1", "presencia_1", "temp_2", "hum_2", "presencia_2"
        )
        for momento, t1, h1, p1, t2, h2, p2 in consulta.iterator():
            local = momento - timedelta(minutes=desfase)
            yield escritor.writerow([
                local.strftime("%Y-%m-%d %H:%M:%S"),
                t1, h1, si_no(p1),
                t2, h2, si_no(p2),
            ])

    respuesta = StreamingHttpResponse(generar(), content_type="text/csv; charset=utf-8")
    respuesta["Content-Disposition"] = 'attachment; filename="lecturas_cuarto_frio.csv"'
    return respuesta


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
    intervalo = intervalo_grafica((hasta - desde).total_seconds())

    # Se recorren todas las lecturas originales para detectar interrupciones de datos
    # (cualquier mensaje cuenta, aunque algún sensor venga con ok falso);
    # al mismo tiempo se promedian por intervalo solo para dibujar la gráfica.
    promediador = Promediador(intervalo)
    interrupciones = []
    anterior = None
    consulta = filas.order_by("recibido_en").values_list("recibido_en", *CAMPOS_GRAFICA)
    for fila in consulta.iterator():
        momento = fila[0]
        if anterior is not None:
            segundos = (momento - anterior).total_seconds()
            if segundos > umbral:
                interrupciones.append({
                    "desde": anterior.isoformat(),
                    "hasta": momento.isoformat(),
                    "segundos": int(segundos),
                })
        anterior = momento
        promediador.sumar(momento, fila[1:])

    # Interrupción abierta: la última lectura es vieja y no ha llegado nada nuevo
    fin = min(hasta, ahora)
    if anterior is not None:
        segundos = (fin - anterior).total_seconds()
        if segundos > umbral:
            interrupciones.append({
                "desde": anterior.isoformat(),
                "hasta": fin.isoformat(),
                "segundos": int(segundos),
            })

    # Mínimo, máximo y promedio de temperatura por sensor, solo con lecturas buenas
    resumen = {}
    for n in SENSORES:
        buenas = Q(**{f"ok_{n}": True})
        resumen[f"temp_{n}_min"] = Min(f"temp_{n}", filter=buenas)
        resumen[f"temp_{n}_max"] = Max(f"temp_{n}", filter=buenas)
        resumen[f"temp_{n}_prom"] = Avg(f"temp_{n}", filter=buenas)
    resumen = filas.aggregate(**resumen)

    ultimas = [
        lectura_a_dict(lectura)
        for lectura in filas.order_by("-recibido_en")[:MAX_TABLA]
    ]

    return JsonResponse({
        "desde": desde.isoformat(),
        "hasta": hasta.isoformat(),
        "total": total,
        "intervalo": intervalo,
        "puntos": promediador.puntos(),
        "interrupciones": interrupciones,
        "resumen": resumen,
        "ultimas": ultimas,
    })