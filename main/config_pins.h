#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

#include "driver/gpio.h"

// --- Cantidad de sensores en el sistema ---
#define NUM_SENSORES_TEMP   4
#define NUM_RADARES         1

// --- Bus RS485 para sensores XY-MD03 ---
#define PIN_RS485_TX        GPIO_NUM_17
#define PIN_RS485_RX        GPIO_NUM_18

// --- Pin del radar HLK-LD2410C (salida OUT) ---
#define PIN_RADAR_1         GPIO_NUM_4

// --- Indicador de estado en placa ---
// GPIO48 controla el LED RGB integrado en la placa v1.3
#define PIN_LED_SISTEMA     GPIO_NUM_48

#endif // CONFIG_PINS_H