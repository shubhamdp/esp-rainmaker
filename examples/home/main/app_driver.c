/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdbool.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <iot_button.h>
#include <app_reset.h>

/* This is the GPIO on which the power will be set */
#define OUTPUT_GPIO_LIGHT   17
#define OUTPUT_GPIO_FAN     18
#define OUTPUT_GPIO_COOLER  19

#define BUTTON_GPIO          0
#define BUTTON_ACTIVE_LEVEL  0

#define WIFI_RESET_BUTTON_TIMEOUT       3
#define FACTORY_RESET_BUTTON_TIMEOUT    10

#define NVS_KEY_FAN_STATE       "s_fan"
#define NVS_KEY_LIGHT_STATE     "s_light"
#define NVS_KEY_COOLER_STATE    "s_cooler"

typedef struct {
    bool light;
    bool fan;
    bool cooler;
} power_state_t;

static power_state_t g_power_state;

static bool nvs_state_get(const char *nvs_key)
{
    assert(nvs_key);
    nvs_handle_t handle;
    uint8_t state = 0;

    if (nvs_open("nvs", NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }
    nvs_get_u8(handle, nvs_key, &state);
    nvs_close(handle);
    return state;
}

static esp_err_t nvs_state_set(const char *nvs_key, bool state)
{
    assert(nvs_key);
    nvs_handle_t handle;

    esp_err_t err = nvs_open("nvs", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    if ((err = nvs_set_u8(handle, nvs_key, (uint8_t)state)) == ESP_OK) {
        nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

void IRAM_ATTR app_driver_set_light_state(bool state)
{
    if(g_power_state.light != state) {
        g_power_state.light = state;
        gpio_set_level(OUTPUT_GPIO_LIGHT, g_power_state.light);
        nvs_state_set(NVS_KEY_LIGHT_STATE, g_power_state.light);
    }
}

void IRAM_ATTR app_driver_set_fan_state(bool state)
{
    if(g_power_state.fan != state) {
        g_power_state.fan = state;
        gpio_set_level(OUTPUT_GPIO_FAN, g_power_state.fan);
        nvs_state_set(NVS_KEY_FAN_STATE, g_power_state.fan);
    }
}

void IRAM_ATTR app_driver_set_cooler_state(bool state)
{
    if(g_power_state.cooler != state) {
        g_power_state.cooler = state;
        gpio_set_level(OUTPUT_GPIO_COOLER, g_power_state.cooler);
        nvs_state_set(NVS_KEY_COOLER_STATE, g_power_state.cooler);
    }
}

bool app_driver_get_light_state(void)
{
    return g_power_state.light;
}

bool app_driver_get_fan_state(void)
{
    return g_power_state.fan;
}

bool app_driver_get_cooler_state(void)
{
    return g_power_state.cooler;
}

static void init_output_gpio(uint32_t pin)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (uint64_t)1 << pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);
}

void app_driver_init(void)
{
    init_output_gpio(OUTPUT_GPIO_LIGHT);
    init_output_gpio(OUTPUT_GPIO_FAN);
    init_output_gpio(OUTPUT_GPIO_COOLER);

    g_power_state.light = nvs_state_get(NVS_KEY_LIGHT_STATE);
    gpio_set_level(OUTPUT_GPIO_LIGHT, g_power_state.light);

    g_power_state.light = nvs_state_get(NVS_KEY_FAN_STATE);
    gpio_set_level(OUTPUT_GPIO_FAN, g_power_state.fan);

    g_power_state.light = nvs_state_get(NVS_KEY_COOLER_STATE);
    gpio_set_level(OUTPUT_GPIO_COOLER, g_power_state.cooler);

    button_handle_t btn_handle = iot_button_create(BUTTON_GPIO, BUTTON_ACTIVE_LEVEL);
    if (btn_handle) {
        /* Register Wi-Fi reset and factory reset functionality on same button */
        app_reset_button_register(btn_handle, WIFI_RESET_BUTTON_TIMEOUT, FACTORY_RESET_BUTTON_TIMEOUT);
    }
}
