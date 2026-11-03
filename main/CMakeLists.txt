idf_component_register(SRCS "main.c" "Built_LED.c" "sensor_temp.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_strip
                    PRIV_REQUIRES esp_driver_gpio esp_driver_uart)
