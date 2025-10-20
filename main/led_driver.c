#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "led_driver.h"

static const char *TAG = "APP_MAIN";

static led_strip_handle_t s_led_strip;
static ld_led_state_e current_led_state = LED_DRV_NONE;

void led_driver_set_state(ld_led_state_e next_led_state)
{
    return;
    uint8_t s_red = 255, s_green = 255, s_blue = 255;
    switch (next_led_state)
    {
    case LED_DRV_NONE:
        s_red = 0;
        s_green = 0;
        s_blue = 0;
        break;
    case LED_DRV_ERROR:
        s_red = 255;
        s_green = 0;
        s_blue = 0;
        break;
    case LED_DRV_CONNECTING:
    //magenta
        s_red = 255;
        s_green = 0;
        s_blue = 255;
        break;
    case LED_DRV_IDLE:
        s_red = 0;
        s_green = 128;
        s_blue = 0;
        break;
    case LED_DRV_HEAT_FAN_ON:
        s_red = 0;
        s_green = 204;
        s_blue = 204;
        break;
    case LED_DRV_FAN_ONLY_ON:
        s_red = 0;
        s_green = 0;
        s_blue = 128;
        break;
    default:
        ESP_LOGI(TAG, "Unexpected led state: %d", next_led_state);
        return;
    }
    ESP_ERROR_CHECK(led_strip_set_pixel(s_led_strip, 0, s_red, s_green, s_blue));
    ESP_ERROR_CHECK(led_strip_refresh(s_led_strip));

    current_led_state = next_led_state;
}

/* TODO add blinking
static void led_driver_task(void *pvParameters)
{
}
*/

esp_err_t led_driver_init()
{
    led_strip_config_t led_strip_conf = {
        .max_leds = CONFIG_EXAMPLE_STRIP_LED_NUMBER,
        .strip_gpio_num = CONFIG_EXAMPLE_STRIP_LED_GPIO,
    };
    led_strip_rmt_config_t rmt_conf = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&led_strip_conf, &rmt_conf, &s_led_strip));
    led_driver_set_state(LED_DRV_NONE);
    // xTaskCreate(led_driver_task, "led_task", 4096, NULL, 5, NULL);
    return ESP_OK;
}
