#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "./app_main.h"
#include "./zigbee_app.h"
#include "./common.h"
#include "led_driver.h"

static const char *TAG = "APP_MAIN";

void onDeviceReboted(void)
{
    ESP_LOGI(TAG, "Device rebooted handler");
    ESP_LOGI(TAG, "LED driver initialization...");
    ESP_ERROR_CHECK(led_driver_init());
}

void app_main(void)
{
    ESP_LOGI(TAG, "App start. Flash initialization...");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI(TAG, "Zigbee initialization...");
    ESP_ERROR_CHECK(zigbee_app_init());
}
