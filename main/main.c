#include <stdint.h>

#include "Built_LED.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "APP";

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    const char *name;
} led_color_t;

static const led_color_t color_cycle[] = {
    {50, 0, 0, "Red"},
    {0, 50, 0, "Green"},
    {0, 0, 50, "Blue"},
    {50, 50, 0, "Yellow"},
    {0, 50, 50, "Cyan"},
    {50, 0, 50, "Magenta"},
    {50, 50, 50, "White"},
};

void app_main(void)
{
    esp_err_t err = Built_LED_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize onboard RGB LED: %s", esp_err_to_name(err));
        return;
    }

    size_t color_index = 0;
    const size_t color_count = sizeof(color_cycle) / sizeof(color_cycle[0]);

    while (1) {
        const led_color_t *color = &color_cycle[color_index];
        ESP_LOGI(TAG, "Displaying %s", color->name);
        Built_LED_set_color(color->red, color->green, color->blue);

        color_index = (color_index + 1) % color_count;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
