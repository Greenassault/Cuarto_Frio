idf_component_register(SRCS "main.c" "Built_LED.c" "sensor_temp.c" "mqtt_publisher.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_strip
                    PRIV_REQUIRES esp_driver_gpio esp_driver_uart nvs_flash esp_netif esp_event protocol_examples_common)