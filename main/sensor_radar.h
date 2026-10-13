#ifndef SENSOR_RADAR_H
#define SENSOR_RADAR_H

#include <stdbool.h>
#include "esp_err.h"
#include "config_pins.h"

typedef struct {
    bool presencia_detectada[NUM_RADARES];
} datos_radar_t;

// Funciones públicas del módulo
esp_err_t radar_init(void);
void radar_actualizar_estado(datos_radar_t *estado);

#endif // SENSOR_RADAR_H