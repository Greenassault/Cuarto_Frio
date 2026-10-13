#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO GPIO_NUM_2

void app_main(void)
{
    ESP_ERROR_CHECK(gpio_reset_pin(LED_GPIO));
    ESP_ERROR_CHECK(gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT));

    int level = 0;
    while (1) {
        ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, level));
        level = !level;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
