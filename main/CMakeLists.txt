idf_component_register(SRCS "main.c" "sensor_temp.c" "sensor_radar.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_driver_gpio)
