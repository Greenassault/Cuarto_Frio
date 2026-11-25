# Formato del mensaje MQTT del cuarto frío

Este archivo es la **única referencia** del mensaje que la ESP32-S3 envía al HMI.
Si cambia el formato, se actualiza aquí primero y luego el firmware
(`main/mqtt_publisher.c`), el HMI (`hmi-django/`) y el simulador
(`hmi-django/simulador_sensor.py`).

Versión: 4 sensores de temperatura (desde el 19 de noviembre de 2026).

## Dónde se publica

- **Topic:** `cuartofrio/datos`
- **Broker:** Mosquitto en la laptop del HMI, puerto 1883 (la dirección se
  configura en el firmware con `idf.py menuconfig` → *Configuracion MQTT*).
- **Frecuencia:** un mensaje cada ~2 a 3 segundos (2 s de espera más el tiempo
  de leer los 4 sensores por RS485).
- **Contenido:** un objeto JSON en una sola línea, en UTF-8.

## Ejemplo

```json
{
  "sensor_id": "cuarto_frio_1",
  "temp_1": 3.8, "hum_1": 68.2, "ok_1": true,
  "temp_2": 4.1, "hum_2": 67.5, "ok_2": true,
  "temp_3": 0.0, "hum_3": 0.0,  "ok_3": false,
  "temp_4": 4.4, "hum_4": 70.1, "ok_4": true,
  "presencia_1": false,
  "presencia_2": true
}
```

En este ejemplo el sensor 3 (Sección C) no respondió: `ok_3` es `false` y sus
valores no se deben usar.

## Campos

| Campo | Tipo | Qué significa |
|---|---|---|
| `sensor_id` | texto | Qué ESP32 manda el mensaje. Hoy siempre `"cuarto_frio_1"`. |
| `temp_N` | número | Temperatura del sensor N en °C, con 1 decimal. |
| `hum_N` | número | Humedad relativa del sensor N en %, con 1 decimal. |
| `ok_N` | `true` / `false` | `true` si el sensor N respondió con una lectura válida. |
| `presencia_1` | `true` / `false` | Radar de presencia 1 (zona de las Secciones A y B). |
| `presencia_2` | `true` / `false` | Radar de presencia 2 (zona de las Secciones C y D). |

N va de 1 a 4.

## Qué sensor es cada número

Los 4 son sensores XY-MD03 (temperatura y humedad) en el mismo bus RS485.

| N | Dirección Modbus | Nombre en el HMI | Presencia que se muestra junto |
|---|---|---|---|
| 1 | 1 | Cuarto frío 1 - Sección A | Presencia 1 |
| 2 | 2 | Cuarto frío 1 - Sección B | Presencia 1 |
| 3 | 3 | Cuarto frío 1 - Sección C | Presencia 2 |
| 4 | 4 | Cuarto frío 1 - Sección D | Presencia 2 |

Las direcciones están en `DIRECCIONES_TEMP` de `main/sensor_temp.c`.
Los nombres del HMI están en `NOMBRES_SECCIONES` de `hmi-django/sensores/views.py`.

## Reglas

1. **Si `ok_N` es `false`, se ignoran `temp_N` y `hum_N`.** El firmware manda
   `0.0` en ese caso, pero ese número no es una medición. El HMI guarda la
   lectura con esos valores vacíos, marca el sensor en rojo
   ("Sensor desconectado") y no lo usa en gráficas ni promedios.
2. El firmware pone `ok_N` en `false` si el sensor no contesta, si la respuesta
   llega dañada (CRC incorrecto) o si el valor está fuera del rango del XY-MD03
   (-40 a 80 °C, 0 a 100 %).
3. **Si falta `ok_N` en el mensaje, se toma como `false`.** Así, un firmware
   viejo que solo manda 2 sensores no hace aparecer lecturas falsas en las
   Secciones C y D.
4. Si falta `temp_N` o `hum_N` (o no es un número), el sensor N se toma como
   `ok_N` falso aunque el mensaje diga `true`.
5. Si falta `sensor_id`, `presencia_1` o `presencia_2`, o el JSON está mal
   escrito, el HMI descarta el mensaje completo y lo anota en la terminal.

## Historial del formato

- **Hasta noviembre 2026:** 2 sensores (`temp_1`, `hum_1`, `temp_2`, `hum_2`,
  `ok_1`, `ok_2`). Las lecturas guardadas de esa época quedan en la base de
  datos con los sensores 3 y 4 vacíos y en `ok` falso.
- **19 de noviembre de 2026:** 4 sensores (`temp_1`…`temp_4`, `hum_1`…`hum_4`,
  `ok_1`…`ok_4`). Los sensores de presencia siguen siendo 2.
