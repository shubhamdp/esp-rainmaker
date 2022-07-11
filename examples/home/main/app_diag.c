/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <esp_wifi.h>
#include <esp_diagnostics.h>
#include <esp_diagnostics_metric.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#define TAG "APP_DIAG"
#define TICK_EVERY_HR  ((3600 * 1000) / portTICK_RATE_MS)

typedef struct {
    TimerHandle_t timer;
} app_data_t;

static app_data_t s_app_data;

static void timer_cb(TimerHandle_t timer)
{
    static uint32_t count = 0;
    if (count++ % 24 == 0) {
        esp_diag_task_snapshot_dump();
    }
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        esp_diag_metric_add_int("wifi", "rssi", ap_info.rssi);
    }
}

void app_diag_init(void)
{
    s_app_data.timer = xTimerCreate("diag_timer", TICK_EVERY_HR, true, NULL, timer_cb);
    if (!s_app_data.timer) {
        return;
    }
    xTimerStart(s_app_data.timer, 0);
}
