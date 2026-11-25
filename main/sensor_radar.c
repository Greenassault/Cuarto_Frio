#include "sensor_radar.h"
#include "esp_log.h"

static const char *TAG = "MODULO_RADAR";

esp_err_t radar_init(void) {
    ESP_LOGI(TAG, "Configurando OUT del radar HLK-LD2410C en GPIO%d", PIN_RADAR_1);

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIN_RADAR_1,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo configurar GPIO%d del radar: %s",
                 PIN_RADAR_1, esp_err_to_name(err));
    }

    return err;
}

void radar_actualizar_estado(datos_radar_t *estado) {
    if (estado == NULL) {
        return;
    }

    estado->presencia_detectada[0] = (gpio_get_level(PIN_RADAR_1) == 1);
}