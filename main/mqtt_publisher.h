#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include "esp_err.h"
#include "sensor_temp.h"

esp_err_t mqtt_publisher_init(void);
void mqtt_publisher_enviar(const datos_temperatura_t *datos);

#endif // MQTT_PUBLISHER_H