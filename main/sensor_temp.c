#include "sensor_temp.h"
#include "esp_log.h"
#include "esp_random.h" // Para simular lecturas antes de tener el hardware

static const char *TAG = "MODULO_TEMP";

esp_err_t temp_init(void) {
    ESP_LOGI(TAG, "Inicializando %d sensores de temperatura...", NUM_SENSORES_TEMP);
    
    // TODO: Cuando lleguen los sensores, colocar aquí la inicialización real
    // (Por ejemplo: i2c_new_master_bus, adc_oneshot_new_unit o bus 1-Wire)
    
    return ESP_OK;
}

esp_err_t temp_leer_todos(datos_temperatura_t *datos) {
    if (datos == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    float suma = 0.0f;

    for (int i = 0; i < NUM_SENSORES_TEMP; i++) {
        // Simulación: genera temperaturas entre 20.0 °C y 25.0 °C
        // TODO: Reemplazar esta línea con la lectura física del sensor 'i'
        datos->grados_c[i] = 20.0f + ((float)(esp_random() % 50) / 10.0f);
        suma += datos->grados_c[i];
    }

    datos->promedio = suma / NUM_SENSORES_TEMP;
    return ESP_OK;
}