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

#include <esp_wifi.h>
#include <json_parser.h>
#include <app/server/Server.h>
#include <lib/core/DataModelTypes.h>

static const char *TAG = "app_main";
static bool init_done = false;
uint16_t light_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;

esp_rmaker_device_t *matter_service;
typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint16_t discriminator;
    char unique_id[32];
} esp_matter_alexa_info_t;

const uint8_t mPAKEVerifierBuf [97] = {
    0xea, 0x5c, 0x2c, 0xfc, 0xfa, 0x7c, 0x9f, 0xb9, 0xe6, 0xf6, 0x5d, 0xc1, 0xfe, 0xf5, 0xf4, 0x6d,
    0x89, 0x11, 0x93, 0xf2, 0xa0, 0x28, 0xce, 0xb5, 0xe3, 0x0e, 0x30, 0x0d, 0xe0, 0xeb, 0x44, 0xc7,
    0x04, 0x2c, 0xd6, 0x2a, 0x01, 0x84, 0xa7, 0x67, 0x77, 0xb0, 0xa2, 0x29, 0x87, 0x29, 0x36, 0xd4,
    0xfe, 0x86, 0xd2, 0x8a, 0xb3, 0x5e, 0xf8, 0xbe, 0x26, 0xb1, 0xa5, 0x14, 0xd3, 0xd8, 0x9b, 0x81,
    0x47, 0xd6, 0x0e, 0x0b, 0xcc, 0x47, 0xb1, 0x9a, 0x2c, 0x6d, 0xc1, 0x58, 0x3a, 0x88, 0xf4, 0x1a,
    0x51, 0x83, 0x28, 0xcf, 0x7f, 0x3d, 0x2a, 0x4e, 0xdd, 0x7d, 0xdc, 0x6e, 0xeb, 0xde, 0x86, 0x30,
    0x6e
};

const uint8_t mSaltBuf [32] = {
    0x63, 0xc4, 0xf8, 0xb7, 0x00, 0xdb, 0x54, 0xa4, 0xe4, 0xa6, 0x98, 0x94, 0x4b, 0x99, 0x69, 0x1f,
    0xcc, 0x62, 0xf6, 0xa5, 0xfc, 0x1d, 0x38, 0xe4, 0x13, 0x5c, 0x8c, 0x48, 0xbe, 0xee, 0xc2, 0x10
};

chip::ByteSpan mPAKEVerifier = chip::ByteSpan(mPAKEVerifierBuf);
chip::ByteSpan mSalt         = chip::ByteSpan(mSaltBuf);

uint16_t mDiscriminator = 15;
uint32_t mIterations = 15000;
uint16_t mVendorId = 65521;
uint16_t mProductId = 32769;

using namespace chip;
FabricIndex mFabricIndex = 0;

esp_err_t esp_matter_get_alexa_info(esp_matter_alexa_info_t *matter_alexa_info)
{
// TODO: Use APIs to retrieve following data
    if (!matter_alexa_info) {
        return ESP_ERR_INVALID_ARG;
    }
    matter_alexa_info->vid = mVendorId;
    matter_alexa_info->pid = mProductId;
    matter_alexa_info->discriminator = mDiscriminator;
    strlcpy(matter_alexa_info->unique_id, "00112233445566778899aabbccddeeff", sizeof(matter_alexa_info->unique_id));
    return ESP_OK;
}

static void esp_matter_start_enhanced_commissioning(intptr_t p)//char *code, int timeout)
{
    chip::CommissioningWindowManager & mgr = chip::Server::GetInstance().GetCommissioningWindowManager();

    // const FabricInfo * fabricInfo = Server::GetInstance().GetFabricTable().FindFabricWithIndex(mFabricIndex);

    Spake2pVerifier verifier;
    verifier.Deserialize(mPAKEVerifier);

    CHIP_ERROR err = mgr.OpenEnhancedCommissioningWindow(System::Clock::Seconds16(CHIP_DEVICE_CONFIG_DISCOVERY_TIMEOUT_SECS),
                                        mDiscriminator, verifier, mIterations, mSalt, mFabricIndex, VendorId::NotSpecified);
	if (err != CHIP_NO_ERROR) {
        ESP_LOGE(TAG, "failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
    }


    ESP_LOGI(TAG, "Starting enhanced commissioning window with code: <--> and timeout: %d sec", CHIP_DEVICE_CONFIG_DISCOVERY_TIMEOUT_SECS);

}

static esp_err_t matter_write_cb(const esp_rmaker_device_t *device,
                                 const esp_rmaker_param_t *param,
                                 const esp_rmaker_param_val_t val,
                                 void *priv_data,
                                 esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(param_name, "Commission-Start") == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s", val.val.s, device_name, param_name);

       //  jparse_ctx_t jctx;
       //  if (json_parse_start(&jctx, val.val.s, strlen(val.val.s)) != 0) {
       //      ESP_LOGE(TAG, "Failed to parse Commissioning Start command");
       //      return ESP_FAIL;
       //  }

       //  int val_size = 0;
       //  char *code = NULL;
       //  if (json_obj_get_strlen(&jctx, "code", &val_size) == 0) {
       //      val_size++; /* For NULL termination */
       //      code = (char *)calloc(1, val_size);
       //      if (!code) {
       //          ESP_LOGE(TAG, "Failed to allocate memory for code");
       //          return ESP_ERR_NO_MEM;
       //      }
       //      json_obj_get_string(&jctx, "code", code, val_size);
       //  } else {
       //      ESP_LOGE(TAG, "Failed to get code in Commissioning Start command");
       //      return ESP_ERR_INVALID_ARG;
       //  }

       //  int timeout = 300; /* Default: 5 min */
       //  json_obj_get_int(&jctx, "timeout", &timeout);
//        esp_matter_start_enhanced_commissioning(); //code, timeout);
        chip::DeviceLayer::PlatformMgr().ScheduleWork(esp_matter_start_enhanced_commissioning, reinterpret_cast<intptr_t>(nullptr));
        // json_parse_end(&jctx);
        esp_rmaker_param_update_and_report(
                esp_rmaker_device_get_param_by_type(matter_service, "esp.param.matter-comm-status"),
                esp_rmaker_int(1));
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_rmaker_device_t *esp_rmaker_matter_alexa_service_create(void)
{
    esp_matter_alexa_info_t matter_alexa_info;
    if (esp_matter_get_alexa_info(&matter_alexa_info) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get Matter Alexa Info");
        return NULL;
    }
    uint8_t eth_mac[6];
    if (esp_wifi_get_mac(WIFI_IF_STA, eth_mac) != ESP_OK) {
        ESP_LOGE(TAG, "Could not fetch MAC address. Please initialise Wi-Fi first");
        return NULL;
    }
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
            eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);

    char info[150];
    snprintf(info, sizeof(info), "{\"mac_addr\":\"%s\",\"vid\":%d, \"pid\":%d,\"discriminator\":%d,\"unique_id\":\"%s\"}",
         mac_str, matter_alexa_info.vid, matter_alexa_info.pid, matter_alexa_info.discriminator, matter_alexa_info.unique_id);

    matter_service = esp_rmaker_service_create("Matter-Commission", "esp.service.matter-comm", NULL);
    if (matter_service) {
        esp_rmaker_device_add_param(matter_service, esp_rmaker_param_create("Commission-Info", "esp.param.matter-comm-info", esp_rmaker_obj(info), PROP_FLAG_READ));
        esp_rmaker_device_add_param(matter_service, esp_rmaker_param_create("Commission-Status", "esp.param.matter-comm-status", esp_rmaker_int(0), PROP_FLAG_READ));
        esp_rmaker_device_add_param(matter_service, esp_rmaker_param_create("Commission-Start", "esp.param.matter-comm-start", esp_rmaker_obj("{}"), PROP_FLAG_WRITE));
        esp_rmaker_device_add_cb(matter_service, matter_write_cb, NULL);
    }
    return matter_service;
}


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

	esp_rmaker_node_add_device(node, esp_rmaker_matter_alexa_service_create());

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

// #if CONFIG_ENABLE_CHIP_SHELL
//     esp_matter::console::diagnostics_register_commands();
//     esp_matter::console::init();
// #endif
    init_done = true;
}
