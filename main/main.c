#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Built_LED.h"
#include "sensor_radar.h"
#include "sensor_temp.h"
#include "mqtt_publisher.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MODO_CAMBIAR_DIRECCION 0
#define DIRECCION_ACTUAL 1
#define DIRECCION_NUEVA  4

static const char *TAG = "APP";

static void tarea_conexion_mqtt(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "Iniciando WiFi/MQTT en segundo plano");
    esp_err_t err = mqtt_publisher_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo iniciar WiFi/MQTT: %s", esp_err_to_name(err));
    }

    vTaskDelete(NULL);
}

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
            Built_LED_set_color(0, 0, 25);
        }
        return false;
    }

    err = radar_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al inicializar el radar: %s", esp_err_to_name(err));
        if (*led_disponible) {
            Built_LED_set_color(0, 0, 25);
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
    if (xTaskCreate(tarea_conexion_mqtt, "mqtt_init", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "No se pudo crear la tarea WiFi/MQTT; los sensores seguiran activos");
    }

    while (1) {
        printf("----------------------------\n");
        datos_temperatura_t datos_temp;
        esp_err_t err = temp_leer_todos(&datos_temp);
        datos_radar_t datos_radar = {0};
        radar_actualizar_estado(&datos_radar);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Fallo al leer el sensor de temperatura: %s",
                     esp_err_to_name(err));
            if (led_disponible) {
                Built_LED_set_color(25, 0, 0);
            }
        } else {
            int sensores_disponibles = 0;
            for (int i = 0; i < NUM_SENSORES_TEMP; i++) {
                const lectura_temperatura_t *sensor = &datos_temp.sensores[i];
                if (sensor->disponible) {
                    sensores_disponibles++;
                    printf("Sensor Temperatura %u: %.1f C, %.1f %%\n", sensor->direccion,
                           sensor->grados_c, sensor->humedad);
                } else {
                    printf("Sensor Temperatura %u: no responde\n", sensor->direccion);
                }
            }
            mqtt_publisher_enviar(&datos_temp);
            if (led_disponible) {
                if (sensores_disponibles == NUM_SENSORES_TEMP) {
                    Built_LED_set_color(0, 25, 0);
                } else if (sensores_disponibles == 0) {
                    Built_LED_set_color(25, 0, 0);
                } else {
                    Built_LED_set_color(25, 25, 0);
                }
            }
        }

        printf("Sensor Presencia %d: %s\n",
               PIN_RADAR_1,
               datos_radar.presencia_detectada[0] ? "detectada" : "no detectada");
        printf("Sensor Presencia %d: %s\n",
               PIN_RADAR_2,
               datos_radar.presencia_detectada[1] ? "detectada" : "no detectada");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
#endif
}