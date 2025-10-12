#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* LED strip configuration */
#define CONFIG_EXAMPLE_STRIP_LED_GPIO   8
#define CONFIG_EXAMPLE_STRIP_LED_NUMBER 1

typedef enum {
    NONE,
    ERROR,
    CONNECTING,
    IDLE,
    HEAT_FAN_ON,
    FAN_ONLY_ON
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

#ifdef __cplusplus
} // extern "C"
#endif
