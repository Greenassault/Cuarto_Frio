#include "Built_LED.h"
#include "config_pins.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "BUILT_LED";

static led_strip_handle_t tira_led = NULL;

esp_err_t Built_LED_init(void) {
    ESP_LOGI(TAG, "Inicializando LED RGB en GPIO48...");

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 0,
        .flags.with_dma = false,
    };

    led_strip_config_t config = {
        .strip_gpio_num = PIN_LED_SISTEMA,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };

    esp_err_t err = led_strip_new_rmt_device(&config, &rmt_config, &tira_led);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar el LED en GPIO48: %s", esp_err_to_name(err));
        return err;
    }

    Built_LED_apagar();
    return ESP_OK;
}

void Built_LED_set_color(uint8_t r, uint8_t g, uint8_t b) {
    if (tira_led != NULL) {
        esp_err_t err = led_strip_set_pixel(tira_led, 0, r, g, b);
        if (err == ESP_OK) {
            err = led_strip_refresh(tira_led);
        }
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "No se pudo actualizar el LED: %s", esp_err_to_name(err));
        }
    }
}

void Built_LED_apagar(void) {
    if (tira_led != NULL) {
        esp_err_t err = led_strip_clear(tira_led);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "No se pudo apagar el LED: %s", esp_err_to_name(err));
        }
    }
}