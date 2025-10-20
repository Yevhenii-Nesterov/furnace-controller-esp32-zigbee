#include "esp_zigbee_core.h"

/* Zigbee configuration (router) */
#define MAX_CHILDREN                    10                                    /* the max amount of connected devices */
#define INSTALLCODE_POLICY_ENABLE       false                                /* enable the install code policy for security */
#define HVAC_ENDPOINT                   0x01                                   /* esp light bulb device endpoint, used to process light controlling commands */
#define ESP_ZB_PRIMARY_CHANNEL_MASK     ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK /* Zigbee primary channel mask use in the example */

/* Basic manufacturer information. Note: the first byte is the length on the string */
#define ESP_MANUFACTURER_NAME "\x03""NYS"      
#define ESP_MODEL_IDENTIFIER "\x0C""FURNACE-CTR1" 

#define CUSTOM_SERVER_ENDPOINT 0x01
#define CUSTOM_CLIENT_ENDPOINT 0x01
#define CUSTOM_CLUSTER_ID 0xff00
#define CUSTOM_COMMAND_RESP 0x0001

#define ESP_ZB_ZR_CONFIG()                                                              \
    {                                                                                   \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,                                       \
        .install_code_policy = INSTALLCODE_POLICY_ENABLE,                               \
        .nwk_cfg.zczr_cfg = {                                                           \
            .max_children = MAX_CHILDREN,                                               \
        },                                                                              \
    }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                           \
    {                                                           \
        .radio_mode = ZB_RADIO_MODE_NATIVE,                     \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                            \
    {                                                           \
        .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,   \
    }

esp_err_t zigbee_app_init(void);
void reset_multi_output_present_value_attribute(void);