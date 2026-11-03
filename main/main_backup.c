#if 0  // Disabled backup implementation; excluded from compilation.
#include <stdint.h>

#include "Built_LED.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Tag used to identify log messages from this application.
static const char *TAG = "APP";

// One RGB color and its human-readable name for logging.
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    const char *name;
} led_color_t;

// Colors shown in sequence. Keep each channel within the 0-255 range
// accepted by the uint8_t fields and the Built_LED_set_color() interface.
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
    // Initialize the onboard RGB LED before attempting to control it.
    esp_err_t err = Built_LED_init();
    if (err != ESP_OK) {
        // Log the initialization failure and stop; later LED calls would be unsafe.
        ESP_LOGE(TAG, "Failed to initialize onboard RGB LED: %s", esp_err_to_name(err));
        return;
    }

    // Start at the first color, and derive the number of entries from the array
    // so this stays correct if colors are added to or removed from color_cycle.
    size_t color_index = 0;
    const size_t color_count = sizeof(color_cycle) / sizeof(color_cycle[0]);

    // Continuously display each color, wrapping back to the first after the last.
    while (1) {
        const led_color_t *color = &color_cycle[color_index];
        // Report the selected color, then apply its red, green, and blue levels.
        ESP_LOGI(TAG, "Displaying %s", color->name);
        Built_LED_set_color(color->red, color->green, color->blue);

        // Advance only after using the current entry; modulo keeps the index
        // within the array bounds on the next loop iteration.
        color_index = (color_index + 1) % color_count;
        // Yield to other FreeRTOS tasks for one second between color changes.
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif
