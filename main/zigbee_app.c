#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"
#include "./zigbee_app.h"
#include "./common.h"
#include "led_driver.h"

#if !defined CONFIG_ZB_ZCZR
#error Define ZB_ZCZR in idf.py menuconfig to compile light (Router) source code.
#endif

#define MULTISTATE_DEFAULT_STATE_VALUE 0

static const char *TAG = "ZB_APP";

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee commissioning");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    switch (sig_type)
    {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK)
        {
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
            if (esp_zb_bdb_is_factory_new())
            {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
                led_driver_set_state(LED_DRV_CONNECTING);
            }
            else
            {
                ESP_LOGI(TAG, "Device rebooted");
            }
        }
        else
        {
            /* commissioning failed */
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
            led_driver_set_state(LED_DRV_ERROR);
        }
        if (ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT == sig_type)
        {
            onDeviceReboted();
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK)
        {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
            led_driver_set_state(LED_DRV_NONE);
        }
        else
        {
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;
    bool light_state = 0;

    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d)", message->info.dst_endpoint, message->info.cluster,
             message->attribute.id, message->attribute.data.size);
    if (message->info.dst_endpoint == HVAC_ENDPOINT)
    {
        if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_MULTI_OUTPUT)
        {
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_MULTI_OUTPUT_PRESENT_VALUE_ID && (message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U16 || message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U8))
            {

                ESP_LOGI(TAG, "Data type 0x%x", message->attribute.data.type);

                /*
                light_state = message->attribute.data.value ? *(bool *)message->attribute.data.value : light_state;
                ESP_LOGI(TAG, "Light sets to %s", light_state ? "On" : "Off");
                led_driver_set_state(light_state ? IDLE : NONE);
                */
            }
        }
    }
    return ret;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id)
    {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

void reset_multi_output_present_value_attribute()
{
    // Acquire Zigbee stack lock before modifying attributes
    esp_zb_lock_acquire(portMAX_DELAY);

    float default_state = MULTISTATE_DEFAULT_STATE_VALUE;

    ESP_ERROR_CHECK(esp_zb_zcl_set_attribute_val(
        HVAC_ENDPOINT,
        ESP_ZB_ZCL_CLUSTER_ID_MULTI_OUTPUT,
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
        ESP_ZB_ZCL_ATTR_MULTI_OUTPUT_PRESENT_VALUE_ID,
        &default_state,
        false));

    // Release Zigbee stack lock after modification
    esp_zb_lock_release();
}

static void esp_zb_task(void *pvParameters)
{
    /* initialize Zigbee stack */
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZR_CONFIG();
    esp_zb_init(&zb_nwk_cfg);
    esp_zb_ieee_addr_t my_addr = {0x28, 0x52, 0x79, 0x96, 0x12, 0x9B, 0xCD, 0xEF};
    esp_zb_set_long_address(my_addr);

    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint = HVAC_ENDPOINT,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_CUSTOM_ATTR_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();

    /* Mandatory clusters */
    esp_zb_cluster_list_add_basic_cluster(cluster_list, esp_zb_basic_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(cluster_list, esp_zb_identify_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_attribute_list_t *basic_cluster = esp_zb_cluster_list_get_cluster(cluster_list, ESP_ZB_ZCL_CLUSTER_ID_BASIC, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, ESP_MANUFACTURER_NAME);
    esp_zb_basic_cluster_add_attr(basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, ESP_MODEL_IDENTIFIER);

    /* multistate output cluster */
    esp_zb_multistate_output_cluster_cfg_t multistate_output_cfg = {
        .number_of_states = 5,
        .out_of_service = false,
        .present_value = MULTISTATE_DEFAULT_STATE_VALUE,
        .status_flags = ESP_ZB_ZCL_MULTI_VALUE_STATUS_FLAGS_NORMAL,
    };

    char default_description[] = "\x11"
                                 "Furnace control state";
    uint32_t application_type = ESP_ZB_ZCL_MO_DOMAIN_HVAC_OFF_ON_AUTO;

    esp_zb_attribute_list_t *multistate_output_cluster = esp_zb_multistate_output_cluster_create(&multistate_output_cfg);

    esp_err_t ret = esp_zb_multistate_output_cluster_add_attr(multistate_output_cluster, ESP_ZB_ZCL_ATTR_MULTI_OUTPUT_DESCRIPTION_ID, (void *)default_description);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add description attribute: 0x%x: %s", ret, esp_err_to_name(ret));
        return;
    }

    ret = esp_zb_multistate_output_cluster_add_attr(multistate_output_cluster, ESP_ZB_ZCL_ATTR_MULTI_OUTPUT_APPLICATION_TYPE_ID, (void *)&application_type);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add application type attribute: 0x%x: %s", ret, esp_err_to_name(ret));
        return;
    }

    ret = esp_zb_cluster_list_add_multistate_output_cluster(cluster_list, multistate_output_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add multistate_output_cluster: 0x%x: %s", ret, esp_err_to_name(ret));
        return;
    }

    esp_zb_ep_list_add_ep(ep_list, cluster_list, endpoint_config);
    esp_zb_device_register(ep_list);
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

esp_err_t zigbee_app_init(void)
{
    esp_err_t ret = ESP_OK;
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    xTaskCreate(esp_zb_task, "Zigbee_app_main", 4096, NULL, 5, NULL);
    return ret;
}
