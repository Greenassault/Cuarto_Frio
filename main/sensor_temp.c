#include "sensor_temp.h"
#include <stddef.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MODULO_TEMP";
static const uart_port_t UART_RS485 = UART_NUM_1;
static const uint8_t DIRECCIONES_TEMP[NUM_SENSORES_TEMP] = {1, 2};
static const int TIEMPO_ESPERA_MS = 300;
static bool uart_inicializada;

static uint16_t crc_modbus(const uint8_t *datos, size_t largo)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < largo; i++) {
        crc ^= datos[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
    }
    return crc;
}

static void agregar_crc(uint8_t *mensaje, size_t largo)
{
    uint16_t crc = crc_modbus(mensaje, largo);
    mensaje[largo] = crc & 0xFF;
    mensaje[largo + 1] = crc >> 8;
}

static bool crc_correcto(const uint8_t *respuesta, size_t largo)
{
    if (largo < 4) {
        return false;
    }

    uint16_t crc = crc_modbus(respuesta, largo - 2);
    return respuesta[largo - 2] == (crc & 0xFF) &&
           respuesta[largo - 1] == (crc >> 8);
}

static int enviar_y_recibir(const uint8_t *mensaje, size_t largo_mensaje,
                            uint8_t *respuesta, size_t largo_respuesta)
{
    esp_err_t err = uart_flush_input(UART_RS485);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo limpiar la UART: %s", esp_err_to_name(err));
        return -1;
    }

    int escritos = uart_write_bytes(UART_RS485, mensaje, largo_mensaje);
    if (escritos != (int)largo_mensaje) {
        ESP_LOGE(TAG, "No se pudo enviar la solicitud Modbus");
        return -1;
    }

    err = uart_wait_tx_done(UART_RS485, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error esperando el envio Modbus: %s", esp_err_to_name(err));
        return -1;
    }

    err = uart_flush_input(UART_RS485);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo descartar el eco UART: %s", esp_err_to_name(err));
        return -1;
    }

    return uart_read_bytes(UART_RS485, respuesta, largo_respuesta,
                           pdMS_TO_TICKS(TIEMPO_ESPERA_MS));
}

static bool leer_sensor(uint8_t direccion, float *temperatura, float *humedad)
{
    uint8_t mensaje[8] = {direccion, 0x04, 0x00, 0x01, 0x00, 0x02};
    agregar_crc(mensaje, 6);

    uint8_t respuesta[9];
    int recibidos = enviar_y_recibir(mensaje, sizeof(mensaje), respuesta,
                                     sizeof(respuesta));
    if (recibidos != sizeof(respuesta) || respuesta[0] != direccion ||
        respuesta[1] != 0x04 || respuesta[2] != 4 ||
        !crc_correcto(respuesta, sizeof(respuesta))) {
        return false;
    }

    int16_t valor_temperatura = (int16_t)((respuesta[3] << 8) | respuesta[4]);
    int16_t valor_humedad = (int16_t)((respuesta[5] << 8) | respuesta[6]);
    *temperatura = valor_temperatura / 10.0f;
    *humedad = valor_humedad / 10.0f;
    return true;
}

bool temp_cambiar_direccion(uint8_t actual, uint8_t nueva)
{
    uint8_t mensaje[8] = {actual, 0x06, 0x01, 0x01, 0x00, nueva};
    agregar_crc(mensaje, 6);

    uint8_t respuesta[8];
    int recibidos = enviar_y_recibir(mensaje, sizeof(mensaje), respuesta,
                                     sizeof(respuesta));
    return recibidos == sizeof(respuesta) && respuesta[0] == actual &&
           respuesta[1] == 0x06 && crc_correcto(respuesta, sizeof(respuesta));
}

esp_err_t temp_init(void)
{
    if (uart_inicializada) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Inicializando bus RS485 para %d sensores...", NUM_SENSORES_TEMP);

    const uart_config_t config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(UART_RS485, 256, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo instalar el controlador UART: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_param_config(UART_RS485, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudo configurar la UART: %s", esp_err_to_name(err));
        uart_driver_delete(UART_RS485);
        return err;
    }

    err = uart_set_pin(UART_RS485, PIN_RS485_TX, PIN_RS485_RX,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No se pudieron configurar los pines RS485: %s", esp_err_to_name(err));
        uart_driver_delete(UART_RS485);
        return err;
    }

    uart_inicializada = true;
    return ESP_OK;
}

esp_err_t temp_leer_todos(datos_temperatura_t *datos)
{
    if (datos == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!uart_inicializada) {
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t i = 0; i < NUM_SENSORES_TEMP; i++) {
        lectura_temperatura_t *sensor = &datos->sensores[i];
        sensor->direccion = DIRECCIONES_TEMP[i];
        sensor->disponible = leer_sensor(sensor->direccion, &sensor->grados_c,
                                         &sensor->humedad);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return ESP_OK;
}