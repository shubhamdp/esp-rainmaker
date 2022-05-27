/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>

#include <app_priv.h>
#include <app_matter.h>
#include <app_matter_utils.h>
#include <app_reset.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <app_insights.h>

static const char *TAG = "app_main";
static bool init_done = false;
uint16_t light_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %d, effect: %d", type, effect_id);
    return ESP_OK;
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        /* Driver update */
        app_driver_handle_t driver_handle = (app_driver_handle_t)priv_data;
        err = app_driver_attribute_update(driver_handle, endpoint_id, cluster_id, attribute_id, val);
    } else if (type == POST_UPDATE) {
        if (!init_done) {
            ESP_LOGI(TAG, "RainMaker init not done. Not processing attribute update");
            return ESP_OK;
        }

        /* RainMaker update */
        const char *device_name = app_rainmaker_get_device_name_from_id(endpoint_id);
        const char *param_name = app_rainmaker_get_param_name_from_id(cluster_id, attribute_id);
        if (!device_name || !param_name) {
            ESP_LOGD(TAG, "Device name or param name not handled");
            return ESP_FAIL;
        }

        const esp_rmaker_node_t *node = esp_rmaker_get_node();
        esp_rmaker_device_t *device = esp_rmaker_node_get_device_by_name(node, device_name);
        esp_rmaker_param_t *param = esp_rmaker_device_get_param_by_name(device, param_name);
        esp_rmaker_param_val_t rmaker_val = app_rainmaker_get_rmaker_val(val, cluster_id, attribute_id);
        if (!param) {
            ESP_LOGE(TAG, "Param not found");
            return ESP_FAIL;
        }

        return esp_rmaker_param_update_and_report(param, rmaker_val);
    }

    return err;
}

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                          const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param);

    uint16_t endpoint_id = app_rainmaker_get_endpoint_id_from_name(device_name);
    uint32_t cluster_id = app_rainmaker_get_cluster_id_from_name(param_name);
    uint32_t attribute_id = app_rainmaker_get_attribute_id_from_name(param_name);
    esp_matter_attr_val_t matter_val = app_rainmaker_get_matter_val((esp_rmaker_param_val_t *)&val, cluster_id,
                                                                    attribute_id);

    return attribute::update(endpoint_id, cluster_id, attribute_id, &matter_val);
}

extern "C" void app_main()
{
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Initialize driver */
    app_driver_handle_t light_handle = app_driver_light_init();
    app_driver_handle_t button_handle = app_driver_button_init();
    app_reset_button_register(button_handle);

    /* Initialize matter */
    app_matter_init(app_attribute_update_cb, app_identification_cb);
    app_matter_light_create(light_handle);

    /* Matter start */
    app_matter_start();

    /* Starting driver with default values */
    app_driver_light_set_defaults(light_endpoint_id);

    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Lightbulb");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node.");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    /* Create the rainmaker device and its params from matter data model */
    app_matter_rainmaker_device_create(write_cb);

    /* Enable OTA */
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_PARAMS);

    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();

    /* Enable scheduling. */
    esp_rmaker_schedule_enable();

    /* Enable Scenes */
    esp_rmaker_scenes_enable();

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    app_insights_enable();

    /* Pre start */
    app_matter_pre_rainmaker_start();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter_console_diagnostics_register_commands();
    esp_matter_console_init();
#endif
    init_done = true;
}
