#ifndef SENSOR_TEMP_H
#define SENSOR_TEMP_H

#include "esp_err.h"
#include "config_pins.h"

// Estructura para almacenar las 6 lecturas juntas
typedef struct {
    float grados_c[NUM_SENSORES_TEMP];
    float promedio;
} datos_temperatura_t;

// Funciones públicas del módulo
esp_err_t temp_init(void);
esp_err_t temp_leer_todos(datos_temperatura_t *datos);

#endif // SENSOR_TEMP_H