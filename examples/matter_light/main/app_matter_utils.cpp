/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <string.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_types.h>

#include <app_priv.h>
#include <app_matter_utils.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_rainmaker.h>

using namespace esp_matter;
using namespace chip::app::Clusters;

static const char *TAG = "app_matter_utils";
extern uint16_t light_endpoint_id;

#define DEFAULT_LIGHT_NAME "Matter Light"

esp_rmaker_param_val_t app_rainmaker_get_rmaker_val(esp_matter_attr_val_t *val, uint32_t cluster_id,
                                                           uint32_t attribute_id)
{
    /* Attributes which need to be remapped */
    if (cluster_id == LevelControl::Id) {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            int value = REMAP_TO_RANGE(val->val.u8, MATTER_BRIGHTNESS, STANDARD_BRIGHTNESS);
            return esp_rmaker_int(value);
        }
    } else if (cluster_id == ColorControl::Id) {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            int value = REMAP_TO_RANGE(val->val.u8, MATTER_HUE, STANDARD_HUE);
            return esp_rmaker_int(value);
        } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            int value = REMAP_TO_RANGE(val->val.u8, MATTER_SATURATION, STANDARD_SATURATION);
            return esp_rmaker_int(value);
        } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            int value = REMAP_TO_RANGE_INVERSE(val->val.u16, STANDARD_TEMPERATURE_FACTOR);
            return esp_rmaker_int(value);
        }
    }

    /* Attributes which don't need to be remapped */
    if (val->type == ESP_MATTER_VAL_TYPE_BOOLEAN) {
        return esp_rmaker_bool(val->val.b);
    } else if (val->type == ESP_MATTER_VAL_TYPE_INTEGER) {
        return esp_rmaker_int(val->val.i);
    } else if (val->type == ESP_MATTER_VAL_TYPE_FLOAT) {
        return esp_rmaker_float(val->val.f);
    } else if (val->type == ESP_MATTER_VAL_TYPE_UINT8) {
        return esp_rmaker_int(val->val.u8);
    } else if (val->type == ESP_MATTER_VAL_TYPE_INT16) {
        return esp_rmaker_int(val->val.i16);
    } else if (val->type == ESP_MATTER_VAL_TYPE_UINT16) {
        return esp_rmaker_int(val->val.u16);
    } else {
        ESP_LOGE(TAG, "Invalid val type: %d", val->type);
    }
    return esp_rmaker_int(0);
}

esp_matter_attr_val_t app_rainmaker_get_matter_val(esp_rmaker_param_val_t *val, uint32_t cluster_id,
                                                          uint32_t attribute_id)
{
    /* Attributes which need to be remapped */
    if (cluster_id == LevelControl::Id) {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            uint8_t value = REMAP_TO_RANGE(val->val.i, STANDARD_BRIGHTNESS, MATTER_BRIGHTNESS);
            return esp_matter_uint8(value);
        }
    } else if (cluster_id == ColorControl::Id) {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            uint8_t value = REMAP_TO_RANGE(val->val.i, STANDARD_HUE, MATTER_HUE);
            return esp_matter_uint8(value);
        } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            uint8_t value = REMAP_TO_RANGE(val->val.i, STANDARD_SATURATION, MATTER_SATURATION);
            return esp_matter_uint8(value);
        } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            uint16_t value = REMAP_TO_RANGE_INVERSE(val->val.i, MATTER_TEMPERATURE_FACTOR);
            return esp_matter_uint16(value);
        }
    }

    /* Attributes which don't need to be remapped */
    if (val->type == RMAKER_VAL_TYPE_BOOLEAN) {
        return esp_matter_bool(val->val.b);
    } else if (val->type == RMAKER_VAL_TYPE_INTEGER) {
        return esp_matter_int(val->val.i);
    } else if (val->type == RMAKER_VAL_TYPE_FLOAT) {
        return esp_matter_float(val->val.f);
    } else {
        ESP_LOGE(TAG, "Invalid val type: %d", val->type);
    }
    return esp_matter_int(0);
}

const char *app_rainmaker_get_device_name_from_id(uint32_t endpoint_id)
{
    if (endpoint_id == light_endpoint_id) {
        return DEFAULT_LIGHT_NAME;
    }
    return NULL;
}

static const char *app_rainmaker_get_device_type_from_id(uint32_t device_type_id)
{
    if (device_type_id == endpoint::color_temperature_light::get_device_type_id()) {
        return ESP_RMAKER_DEVICE_LIGHTBULB;
    }
    return NULL;
}

uint16_t app_rainmaker_get_endpoint_id_from_name(const char *device_name)
{
    if (strcmp(device_name, DEFAULT_LIGHT_NAME) == 0) {
        return light_endpoint_id;
    }
    return 0xFFFF;
}

const char *app_rainmaker_get_param_name_from_id(uint32_t cluster_id, uint32_t attribute_id)
{
    if (cluster_id == OnOff::Id) {
        if (attribute_id == OnOff::Attributes::OnOff::Id) {
            return ESP_RMAKER_DEF_POWER_NAME;
        }
    } else if (cluster_id == LevelControl::Id) {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            return ESP_RMAKER_DEF_BRIGHTNESS_NAME;
        }
    } else if (cluster_id == ColorControl::Id) {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            return ESP_RMAKER_DEF_HUE_NAME;
        } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            return ESP_RMAKER_DEF_SATURATION_NAME;
        } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            return ESP_RMAKER_DEF_CCT_NAME;
        }
    }
    return NULL;
}

static const char *app_rainmaker_get_param_type_from_id(uint32_t cluster_id, uint32_t attribute_id)
{
    if (cluster_id == OnOff::Id) {
        if (attribute_id == OnOff::Attributes::OnOff::Id) {
            return ESP_RMAKER_PARAM_POWER;
        }
    } else if (cluster_id == LevelControl::Id) {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            return ESP_RMAKER_PARAM_BRIGHTNESS;
        }
    } else if (cluster_id == ColorControl::Id) {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            return ESP_RMAKER_PARAM_HUE;
        } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            return ESP_RMAKER_PARAM_SATURATION;
        } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            return ESP_RMAKER_PARAM_CCT;
        }
    }
    return NULL;
}

static const char *app_rainmaker_get_param_ui_type_from_id(uint32_t cluster_id, uint32_t attribute_id)
{
    if (cluster_id == OnOff::Id) {
        if (attribute_id == OnOff::Attributes::OnOff::Id) {
            return ESP_RMAKER_UI_TOGGLE;
        }
    } else if (cluster_id == LevelControl::Id) {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            return ESP_RMAKER_UI_SLIDER;
        }
    } else if (cluster_id == ColorControl::Id) {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            return ESP_RMAKER_UI_HUE_SLIDER;
        } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            return ESP_RMAKER_UI_SLIDER;
        } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            return ESP_RMAKER_UI_SLIDER;
        }
    }
    return NULL;
}

static bool app_rainmaker_get_param_bounds_from_id(uint32_t cluster_id, uint32_t attribute_id, int *min, int *max,
                                                   int *step)
{
    if (cluster_id == LevelControl::Id) {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            *min = 0;
            *max = STANDARD_BRIGHTNESS;
            *step = 1;
            return true;
        }
    } else if (cluster_id == ColorControl::Id) {
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            *min = 0;
            *max = STANDARD_HUE;
            *step = 1;
            return true;
        } else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            *min = 0;
            *max = STANDARD_SATURATION;
            *step = 1;
            return true;
        } else if (attribute_id == ColorControl::Attributes::ColorTemperatureMireds::Id) {
            *min = 2700;
            *max = 6500;
            *step = 100;
            return true;
        }
    }
    return false;
}

static esp_err_t app_rainmaker_param_add_ui_type(esp_rmaker_param_t *param, cluster_t *cluster, attribute_t *attribute)
{
    uint32_t cluster_id = cluster::get_id(cluster);
    uint32_t attribute_id = attribute::get_id(attribute);
    const char *ui_type = app_rainmaker_get_param_ui_type_from_id(cluster_id, attribute_id);
    if (!ui_type) {
        return ESP_OK;
    }
    return esp_rmaker_param_add_ui_type(param, ui_type);
}

static esp_err_t app_rainmaker_param_add_bounds(esp_rmaker_param_t *param, cluster_t *cluster, attribute_t *attribute)
{
    uint32_t cluster_id = cluster::get_id(cluster);
    uint32_t attribute_id = attribute::get_id(attribute);
    esp_matter_attr_bounds_t *bounds = attribute::get_bounds(attribute);
    if (bounds) {
        esp_rmaker_param_val_t min_val = app_rainmaker_get_rmaker_val(&bounds->min, cluster_id, attribute_id);
        esp_rmaker_param_val_t max_val = app_rainmaker_get_rmaker_val(&bounds->max, cluster_id, attribute_id);
        esp_rmaker_param_val_t step_val = esp_rmaker_int(1);
        return esp_rmaker_param_add_bounds(param, min_val, max_val, step_val);
    }

    /* If bounds are not set for the attribute, check if there are any bounds to be added based on the id */
    int min = 0, max = 0, step = 0;
    bool add_bounds = app_rainmaker_get_param_bounds_from_id(cluster_id, attribute_id, &min, &max, &step);
    if (add_bounds) {
        esp_rmaker_param_val_t min_val = esp_rmaker_int(min);
        esp_rmaker_param_val_t max_val = esp_rmaker_int(max);
        esp_rmaker_param_val_t step_val = esp_rmaker_int(step);
        return esp_rmaker_param_add_bounds(param, min_val, max_val, step_val);
    }

    return ESP_OK;
}

uint32_t app_rainmaker_get_cluster_id_from_name(const char *param_name)
{
    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        return OnOff::Id;
    } else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0) {
        return LevelControl::Id;
    } else if (strcmp(param_name, ESP_RMAKER_DEF_HUE_NAME) == 0) {
        return ColorControl::Id;
    } else if (strcmp(param_name, ESP_RMAKER_DEF_SATURATION_NAME) == 0) {
        return ColorControl::Id;
    } else if (strcmp(param_name, ESP_RMAKER_DEF_CCT_NAME) == 0) {
        return ColorControl::Id;
    }
    return 0;
}

uint32_t app_rainmaker_get_attribute_id_from_name(const char *param_name)
{
    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        return OnOff::Attributes::OnOff::Id;
    } else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0) {
        return LevelControl::Attributes::CurrentLevel::Id;
    } else if (strcmp(param_name, ESP_RMAKER_DEF_HUE_NAME) == 0) {
        return ColorControl::Attributes::CurrentHue::Id;
    } else if (strcmp(param_name, ESP_RMAKER_DEF_SATURATION_NAME) == 0) {
        return ColorControl::Attributes::CurrentSaturation::Id;
    } else if (strcmp(param_name, ESP_RMAKER_DEF_CCT_NAME) == 0) {
        return ColorControl::Attributes::ColorTemperatureMireds::Id;
    }
    return 0;
}

static esp_rmaker_device_t *app_rainmaker_device_create(const esp_rmaker_node_t *node, endpoint_t *endpoint,
                                                        esp_rmaker_device_write_cb_t write_cb)
{
    uint16_t endpoint_id = endpoint::get_id(endpoint);
    const char *device_name = app_rainmaker_get_device_name_from_id(endpoint_id);
    if (!device_name) {
        return NULL;
    }
    /* Add this device only if endpoint_id has been handled */
    uint8_t device_type_count = 0;
    uint32_t *device_type_id = endpoint::get_device_type_ids(endpoint, &device_type_count);
    const char *device_type = app_rainmaker_get_device_type_from_id(*device_type_id);
    esp_rmaker_device_t *device = esp_rmaker_device_create(device_name, device_type, NULL);
    if (!device) {
        ESP_LOGE(TAG, "Could not create rainmaker device");
        return NULL;
    }
    esp_rmaker_device_add_cb(device, write_cb, NULL);
    esp_rmaker_node_add_device(node, device);
    return device;
}

static esp_rmaker_param_t *app_rainmaker_param_create(esp_rmaker_device_t *device, cluster_t *cluster,
                                                      attribute_t *attribute)
{
    uint32_t cluster_id = cluster::get_id(cluster);
    uint32_t attribute_id = attribute::get_id(attribute);
    const char *param_name = app_rainmaker_get_param_name_from_id(cluster_id, attribute_id);
    if (!param_name) {
        return NULL;
    }
    /* Add this param only if attribute_id corresponding to the cluster_id is handled */
    const char *param_type = app_rainmaker_get_param_type_from_id(cluster_id, attribute_id);
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attribute, &val);
    esp_rmaker_param_val_t rmaker_val = app_rainmaker_get_rmaker_val(&val, cluster_id, attribute_id);
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, param_type, rmaker_val,
                                                        PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (!param) {
        ESP_LOGE(TAG, "Could not create rainmaker param");
        return NULL;
    }
    /* Add additional param details */
    app_rainmaker_param_add_ui_type(param, cluster, attribute);
    app_rainmaker_param_add_bounds(param, cluster, attribute);
    esp_rmaker_device_add_param(device, param);
    if (strcmp(param_type, ESP_RMAKER_PARAM_POWER) == 0) {
        esp_rmaker_device_assign_primary_param(device, param);
    }
    return param;
}

void app_matter_rainmaker_device_create(esp_rmaker_device_write_cb_t write_cb)
{
    const esp_rmaker_node_t *node = esp_rmaker_get_node();
    node_t *matter_node = node::get();
    endpoint_t *endpoint = endpoint::get_first(matter_node);

    /* Parse all endpoints */
    while (endpoint) {
        esp_rmaker_device_t *device = app_rainmaker_device_create(node, endpoint, write_cb);
        /* Proceed only if the device has been handled */
        if (device) {
            cluster_t *cluster = cluster::get_first(endpoint);
            /* Parse all clusters */
            while (cluster) {
                attribute_t *attribute = attribute::get_first(cluster);
                /* Parse all attributes */
                while (attribute) {
                    app_rainmaker_param_create(device, cluster, attribute);
                    attribute = attribute::get_next(attribute);
                }
                cluster = cluster::get_next(cluster);
            }
        }
        endpoint = endpoint::get_next(endpoint);
    }
}
