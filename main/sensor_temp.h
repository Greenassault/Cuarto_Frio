#ifndef SENSOR_TEMP_H
#define SENSOR_TEMP_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "config_pins.h"

typedef struct {
    uint8_t direccion;
    float grados_c;
    float humedad;
    bool disponible;
} lectura_temperatura_t;

typedef struct {
    lectura_temperatura_t sensores[NUM_SENSORES_TEMP];
} datos_temperatura_t;

esp_err_t temp_init(void);
esp_err_t temp_leer_todos(datos_temperatura_t *datos);
bool temp_cambiar_direccion(uint8_t actual, uint8_t nueva);

#endif // SENSOR_TEMP_H