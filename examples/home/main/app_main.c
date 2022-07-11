/* Switch Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_mqtt.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_diagnostics.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>

#include <app_wifi.h>
#include "app_priv.h"

#define DEVICE_LIGHT    "Light"
#define DEVICE_FAN      "Fan"
#define DEVICE_COOLER   "Cooler"

static const char *TAG = "app_main";

/* Callback to handle commands received from the RainMaker cloud */
static esp_err_t device_wr_cb(const esp_rmaker_device_t *device,
                              const esp_rmaker_param_t *param,
                              const esp_rmaker_param_val_t val,
                              void *priv_data,
                              esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    ESP_LOGI(TAG, "Received value = %s for %s - %s",
                  val.val.b? "true" : "false",
                  esp_rmaker_device_get_name(device),
                  esp_rmaker_param_get_name(param));

    if (strcmp(esp_rmaker_param_get_name(param), ESP_RMAKER_DEF_POWER_NAME) == 0) {
        const char *device_name = esp_rmaker_device_get_name(device);
        if (strcmp(device_name, DEVICE_LIGHT) == 0) {
            app_driver_set_light_state(val.val.b);
        } else if (strcmp(device_name, DEVICE_FAN) == 0) {
            app_driver_set_fan_state(val.val.b);
        } else if (strcmp(device_name, DEVICE_COOLER) == 0) {
            app_driver_set_cooler_state(val.val.b);
        }
        esp_rmaker_param_update_and_report(param, val);
    }
    return ESP_OK;
}

void app_main()
{
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Initialize Application specific hardware drivers and set initial state. */
    app_driver_init();

    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init() */
    app_wifi_init();

    /* Initialize the ESP RainMaker Agent */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = true,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Switch");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Light switch */
    esp_rmaker_device_t *shed_light = esp_rmaker_switch_device_create(DEVICE_LIGHT, NULL, app_driver_get_light_state());
    esp_rmaker_device_add_cb(shed_light, device_wr_cb, NULL);
    esp_rmaker_node_add_device(node, shed_light);

    /* Fan switch */
    esp_rmaker_device_t *shed_fan = esp_rmaker_switch_device_create(DEVICE_FAN, NULL, app_driver_get_fan_state());
    esp_rmaker_device_add_cb(shed_fan, device_wr_cb, NULL);
    esp_rmaker_node_add_device(node, shed_fan);

    /* Cooler switch */
    esp_rmaker_device_t *shed_cooler = esp_rmaker_switch_device_create(DEVICE_COOLER, NULL, app_driver_get_cooler_state());
    esp_rmaker_device_add_cb(shed_cooler, device_wr_cb, NULL);
    esp_rmaker_node_add_device(node, shed_cooler);

    /* Enable OTA */
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_PARAMS);

    /* Enable scheduling.
     * Please note that you also need to set the timezone for schedules to work correctly.
     * Simplest option is to use the CONFIG_ESP_RMAKER_DEF_TIMEZONE config option.
     * Else, you can set the timezone using the API call `esp_rmaker_time_set_timezone("Asia/Shanghai");`
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_schedule_enable();

    esp_rmaker_mqtt_config_t mqtt_config = {
        .init           = NULL,
        .connect        = NULL,
        .disconnect     = NULL,
        .publish        = esp_rmaker_mqtt_publish,
        .subscribe      = esp_rmaker_mqtt_subscribe,
        .unsubscribe    = esp_rmaker_mqtt_unsubscribe,
    };
    esp_rmaker_diag_mqtt_setup(mqtt_config);

    /* Enable diagnostics */
    esp_rmaker_diag_config_t config = {
        .log_type = ESP_DIAG_LOG_TYPE_ERROR | ESP_DIAG_LOG_TYPE_WARNING | ESP_DIAG_LOG_TYPE_EVENT,
        .cloud_reporting_period = 60,
    };
    esp_rmaker_diag_enable(&config);
    app_diag_init();

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    /* Start the Wi-Fi.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
}
