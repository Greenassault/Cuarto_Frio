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
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "No se pudo borrar NVS: %s", esp_err_to_name(err));
            return err;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar esp-netif: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo crear el loop de eventos: %s", esp_err_to_name(err));
        return err;
    }

    err = example_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo inicializar WiFi: %s", esp_err_to_name(err));
        return err;
    }

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
    client = esp_mqtt_client_init(&cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "No se pudo crear el cliente MQTT");
        return ESP_FAIL;
    }

    err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                         mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo registrar eventos MQTT: %s", esp_err_to_name(err));
        esp_mqtt_client_destroy(client);
        client = NULL;
        return err;
    }

    err = esp_mqtt_client_start(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo iniciar el cliente MQTT: %s", esp_err_to_name(err));
        esp_mqtt_client_destroy(client);
        client = NULL;
    }
    return err;
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

// Arma el JSON del topic cuartofrio/datos. El formato está documentado en
// FORMATO_MQTT.md (raíz del repo): temp_N, hum_N y ok_N para cada sensor
// (N = 1..NUM_SENSORES_TEMP, Secciones A a D) y presencia_1, presencia_2.
void mqtt_publisher_enviar(const datos_temperatura_t *datos)
{
    if (!mqtt_conectado) {
        return;
    }

    // Unos 50 caracteres por sensor más el inicio y el final: 512 sobra para 4.
    char msg[512];
    size_t largo = 0;
    int escritos = snprintf(msg, sizeof(msg), "{\"sensor_id\": \"%s\", ", SENSOR_ID);

    for (int i = 0; i < NUM_SENSORES_TEMP && escritos >= 0; i++) {
        largo += escritos;
        if (largo >= sizeof(msg)) {
            break;
        }
        float t, h;
        bool ok;
        valores_sensor(datos, i, &t, &h, &ok);
        escritos = snprintf(msg + largo, sizeof(msg) - largo,
                            "\"temp_%d\": %.1f, \"hum_%d\": %.1f, \"ok_%d\": %s, ",
                            i + 1, t, i + 1, h, i + 1, ok ? "true" : "false");
    }

    if (escritos >= 0 && largo + escritos < sizeof(msg)) {
        largo += escritos;
        // TODO: cuando se conecten los radares, mandar su lectura real.
        escritos = snprintf(msg + largo, sizeof(msg) - largo,
                            "\"presencia_1\": false, \"presencia_2\": false}");
    }

    if (escritos < 0 || largo + escritos >= sizeof(msg)) {
        ESP_LOGE(TAG, "El mensaje MQTT no cupo en el buffer; no se publica");
        return;
    }

    esp_mqtt_client_publish(client, MQTT_TOPIC, msg, 0, 0, 0);
    ESP_LOGI(TAG, "Publicado: %s", msg);
}
