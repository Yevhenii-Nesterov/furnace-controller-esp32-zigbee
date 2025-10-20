#include "esp_check.h"

#define FURNACE_DISCONNECT_GPIO 18
#define FURNACE_FAN_GPIO 19
#define FURNACE_HEAT_GPIO 20

typedef enum {
    FURNACE_DRV_DISCONECTED = 0x00,
    FURNACE_DRV_OFF,
    FURNACE_DRV_FAN_ON,
    FURNACE_DRV_HEAT_AND_FAN_ON,
    FURNACE_DRV_FAILURE,
} furnace_driver_state_e;

void set_furnace_state(furnace_driver_state_e new_state);

esp_err_t furnace_driver_init(void);