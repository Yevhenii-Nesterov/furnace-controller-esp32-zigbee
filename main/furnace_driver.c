#include "./furnace_driver.h"
#include "driver/gpio.h"
#include "esp_log.h"

static struct
{
    furnace_driver_state_e current_state;
    uint64_t state_time_elapsed;
} furnace;

static const char *TAG = "FURNACE_DRV";

void reset_time_elapsed()
{
    furnace.state_time_elapsed = 0;
}

void set_furnace_state(furnace_driver_state_e new_state)
{
    reset_time_elapsed();
    if (furnace.current_state == new_state)
    {
        return;
    }
    uint8_t disconnect_pin_level = 0;
    uint8_t fan_pin_level = 0;
    uint8_t heat_pin_level = 0;
    switch (new_state)
    {
        case FURNACE_DRV_FAILURE: // falls through
        case FURNACE_DRV_DISCONECTED:
            /* all zeros */
            break;
        case FURNACE_DRV_OFF:
            disconnect_pin_level = 1;
            break;
        case FURNACE_DRV_FAN_ON:
            disconnect_pin_level = 1;
            fan_pin_level = 1;
            break;
        case FURNACE_DRV_HEAT_AND_FAN_ON:
            disconnect_pin_level = 1;
            // fan pin should be off, furnace automatics handles the fan operations
            heat_pin_level = 1;
            break;
        default:
        ESP_LOGW(TAG, "Unknown state for set_furnace_state (0x%x)", new_state);
        return;
    }

    gpio_set_level(FURNACE_DISCONNECT_GPIO, disconnect_pin_level);
    gpio_set_level(FURNACE_FAN_GPIO, fan_pin_level);
    gpio_set_level(FURNACE_HEAT_GPIO, heat_pin_level);
    furnace.current_state = new_state;
}

esp_err_t furnace_driver_init(void)
{

    esp_err_t ret;

    ret = gpio_pulldown_en(FURNACE_DISCONNECT_GPIO);
    if (ret != ESP_OK)
    {
        return ret;
    }
    ret = gpio_pulldown_en(FURNACE_FAN_GPIO);
    if (ret != ESP_OK)
    {
        return ret;
    }
    ret = gpio_pulldown_en(FURNACE_HEAT_GPIO);
    if (ret != ESP_OK)
    {
        return ret;
    }

    furnace.current_state = FURNACE_DRV_DISCONECTED;
    furnace.state_time_elapsed = 0;

    return ret;
}