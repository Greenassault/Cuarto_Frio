#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

#include "driver/gpio.h"

// --- Cantidad de sensores en el sistema ---
#define NUM_SENSORES_TEMP   6
#define NUM_RADARES         2

// --- Pines asignados a los 2 Radares (Entradas digitales o interrupciones) ---
// Ubicados en el cabezal J1 (GPIO4 y GPIO5)
#define PIN_RADAR_1         GPIO_NUM_4
#define PIN_RADAR_2         GPIO_NUM_5

// Si los radares usan UART adicional (UART1 / UART2):
#define PIN_RADAR1_TX       GPIO_NUM_17
#define PIN_RADAR1_RX       GPIO_NUM_18
#define PIN_RADAR2_TX       GPIO_NUM_15
#define PIN_RADAR2_RX       GPIO_NUM_16

// --- Pines asignados a Sensores de Temperatura ---
// Opción A: Si usan bus I2C (ej. través de multiplexor o dos buses I2C)
#define PIN_I2C_SDA         GPIO_NUM_8
#define PIN_I2C_SCL         GPIO_NUM_9

// Opción B: Si son analógicos o individuales (GPIO6, 7, 10, 11, 12, 13 en cabezal J1)
#define PIN_TEMP_1          GPIO_NUM_6
#define PIN_TEMP_2          GPIO_NUM_7
#define PIN_TEMP_3          GPIO_NUM_10
#define PIN_TEMP_4          GPIO_NUM_11
#define PIN_TEMP_5          GPIO_NUM_12
#define PIN_TEMP_6          GPIO_NUM_13

// --- Indicador de estado en placa ---
// GPIO48 controla el LED RGB integrado en la placa v1.3
#define PIN_LED_SISTEMA     GPIO_NUM_48

#endif // CONFIG_PINS_H