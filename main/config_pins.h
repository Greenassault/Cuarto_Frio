#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

#include "driver/gpio.h"

// --- Cantidad de sensores en el sistema ---
#define NUM_SENSORES_TEMP   4
#define NUM_RADARES         1

// --- Bus RS485 para sensores XY-MD03 ---
// Los 4 sensores comparten estos dos pines; cada uno se distingue por su
// dirección Modbus (DIRECCIONES_TEMP en sensor_temp.c).
// Cableado: GPIO18 va al pin RX del modulo y GPIO17 a su pin TX. Por eso el
// TX del ESP32 es GPIO18 y el RX del ESP32 es GPIO17. Con esta asignacion el
// equipo confirmo lecturas correctas en el hardware (25 nov 2026).
#define PIN_RS485_TX        GPIO_NUM_18
#define PIN_RS485_RX        GPIO_NUM_17

// --- Pin del radar HLK-LD2410C (salida OUT) ---
#define PIN_RADAR_1         GPIO_NUM_4

// --- Indicador de estado en placa ---
// GPIO48 controla el LED RGB integrado en la placa v1.3
#define PIN_LED_SISTEMA     GPIO_NUM_48

#endif // CONFIG_PINS_H