#include "sensor_radar.h"
#include "esp_log.h"

static const char *TAG = "MODULO_RADAR";

esp_err_t radar_init(void) {
    ESP_LOGI(TAG, "Configurando OUT de %d radares HLK-LD2410C", NUM_RADARES);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_RADAR_1) | (1ULL << PIN_RADAR_2),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudieron configurar los GPIO de los radares: %s",
                 esp_err_to_name(err));
    }

    return err;
}

void radar_actualizar_estado(datos_radar_t *estado) {
    if (estado == NULL) {
        return;
    }

    estado->presencia_detectada[0] = gpio_get_level(PIN_RADAR_1) == 1;
    estado->presencia_detectada[1] = gpio_get_level(PIN_RADAR_2) == 1;
}