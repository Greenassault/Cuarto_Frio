#ifndef BUILT_LED_H
#define BUILT_LED_H

#include <stdint.h>
#include "esp_err.h"

// Inicializa el controlador RMT para el LED RGB integrado en GPIO48
esp_err_t Built_LED_init(void);

// Cambia el color del LED (valores de 0 a 255)
void Built_LED_set_color(uint8_t r, uint8_t g, uint8_t b);

// Apaga por completo el LED RGB
void Built_LED_apagar(void);

#endif // BUILT_LED_H