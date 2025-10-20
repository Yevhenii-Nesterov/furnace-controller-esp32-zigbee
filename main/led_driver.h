#pragma once

#include <stdbool.h>
#include "esp_err.h"


/* LED strip configuration */
#define CONFIG_EXAMPLE_STRIP_LED_GPIO   8
#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 1

typedef enum {
    LED_DRV_NONE,
    LED_DRV_ERROR,
    LED_DRV_CONNECTING,
    LED_DRV_IDLE,
    LED_DRV_HEAT_FAN_ON,
    LED_DRV_FAN_ONLY_ON
} ld_led_state_e;

/**
* @brief Set light power (on/off).
*
* @param  power  The light power to be set
*/
void led_driver_set_state(ld_led_state_e state);

/**
* @brief color light driver init, be invoked where you want to use color light
*
* @param power power on/off
*/
esp_err_t led_driver_init();