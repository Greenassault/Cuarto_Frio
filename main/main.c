#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Built_LED.h"
#include "sensor_temp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MODO_CAMBIAR_DIRECCION 0
#define DIRECCION_ACTUAL 1
#define DIRECCION_NUEVA  2

static const char *TAG = "APP";

static bool inicializar_modulos(bool *led_disponible)
{
    esp_err_t err = Built_LED_init();
    *led_disponible = err == ESP_OK;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al inicializar el LED integrado: %s",
                 esp_err_to_name(err));
    }

    err = temp_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al inicializar el sensor de temperatura: %s",
                 esp_err_to_name(err));
        if (*led_disponible) {
            Built_LED_set_color(50, 0, 0);
        }
        return false;
    }

    return true;
}

void app_main(void)
{
    bool led_disponible = false;
    if (!inicializar_modulos(&led_disponible)) {
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

#if MODO_CAMBIAR_DIRECCION
    printf("Cambiando direccion %d -> %d ...\n", DIRECCION_ACTUAL, DIRECCION_NUEVA);
    if (temp_cambiar_direccion(DIRECCION_ACTUAL, DIRECCION_NUEVA)) {
        printf("Listo. El sensor ahora tiene la direccion %d.\n", DIRECCION_NUEVA);
    } else {
        printf("El sensor no respondio. Revisa conexiones y que solo haya uno conectado.\n");
    }
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#else
    while (1) {
        printf("----------------------------\n");
        datos_temperatura_t datos_temp;
        esp_err_t err = temp_leer_todos(&datos_temp);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Fallo al leer el sensor de temperatura: %s",
                     esp_err_to_name(err));
            if (led_disponible) {
                Built_LED_set_color(50, 0, 0);
            }
        } else {
            int sensores_disponibles = 0;
            for (int i = 0; i < NUM_SENSORES_TEMP; i++) {
                const lectura_temperatura_t *sensor = &datos_temp.sensores[i];
                if (sensor->disponible) {
                    sensores_disponibles++;
                    printf("Sensor %u: %.1f C, %.1f %%\n", sensor->direccion,
                           sensor->grados_c, sensor->humedad);
                } else {
                    printf("Sensor %u: no responde\n", sensor->direccion);
                }
            }
            if (led_disponible) {
                if (sensores_disponibles == NUM_SENSORES_TEMP) {
                    Built_LED_set_color(0, 50, 0);
                } else if (sensores_disponibles == 0) {
                    Built_LED_set_color(50, 0, 0);
                } else {
                    Built_LED_set_color(50, 30, 0);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
#endif
}