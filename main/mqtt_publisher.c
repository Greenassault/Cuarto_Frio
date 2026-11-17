#include <stdio.h>
#include <stdbool.h>
#include "mqtt_publisher.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "protocol_examples_common.h"

#define MQTT_TOPIC "cuartofrio/datos"
#define SENSOR_ID  "cuarto_frio_1"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client = NULL;
static volatile bool mqtt_conectado = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Conectado al broker MQTT");
        mqtt_conectado = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Desconectado del broker MQTT");
        mqtt_conectado = false;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "Error de MQTT");
        break;
    default:
        break;
    }
}

esp_err_t mqtt_publisher_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
    client = esp_mqtt_client_init(&cfg);
    if (client == NULL) {
        return ESP_FAIL;
    }
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    return esp_mqtt_client_start(client);
}

static void valores_sensor(const datos_temperatura_t *d, int i, float *temp, float *hum, bool *ok)
{
    *temp = 0.0f;
    *hum = 0.0f;
    *ok = false;

    if (i >= NUM_SENSORES_TEMP || !d->sensores[i].disponible) {
        return;
    }

    float t = d->sensores[i].grados_c;
    float h = d->sensores[i].humedad;

    // El XY-MD03 mide de -40 a 80 °C y de 0 a 100 %. Fuera de ese rango
    // (por ejemplo -3276.8, que es su código de error) se considera falla.
    if (t < -40.0f || t > 80.0f || h < 0.0f || h > 100.0f) {
        return;
    }

    *temp = t;
    *hum = h;
    *ok = true;
}

void mqtt_publisher_enviar(const datos_temperatura_t *datos)
{
    if (!mqtt_conectado) {
        return;
    }

    float t1, h1, t2, h2;
    bool ok1, ok2;
    valores_sensor(datos, 0, &t1, &h1, &ok1);
    valores_sensor(datos, 1, &t2, &h2, &ok2);

    char msg[320];
    snprintf(msg, sizeof(msg),
             "{\"sensor_id\": \"%s\", "
             "\"temp_1\": %.1f, \"hum_1\": %.1f, "
             "\"temp_2\": %.1f, \"hum_2\": %.1f, "
             "\"presencia_1\": false, \"presencia_2\": false, "
             "\"ok_1\": %s, \"ok_2\": %s}",
             SENSOR_ID, t1, h1, t2, h2,
             ok1 ? "true" : "false", ok2 ? "true" : "false");

    esp_mqtt_client_publish(client, MQTT_TOPIC, msg, 0, 0, 0);
    ESP_LOGI(TAG, "Publicado: %s", msg);
}
