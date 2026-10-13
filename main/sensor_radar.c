#include "sensor_radar.h"
#include "esp_log.h"

static const char *TAG = "MODULO_RADAR";

esp_err_t radar_init(void) {
    ESP_LOGI(TAG, "Configurando pines para %d radares...", NUM_RADARES);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_RADAR_1) | (1ULL << PIN_RADAR_2),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    return gpio_config(&io_conf);
}

void radar_actualizar_estado(datos_radar_t *estado) {
    if (estado == NULL) return;

    // Lectura directa de los pines configurados en config_pines.h
    estado->presencia_detectada[0] = (gpio_get_level(PIN_RADAR_1) == 1);
    estado->presencia_detectada[1] = (gpio_get_level(PIN_RADAR_2) == 1);
}