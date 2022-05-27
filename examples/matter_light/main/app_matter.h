/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <esp_err.h>
#include <esp_matter.h>
#include <esp_rmaker_core.h>
#include <app_priv.h>

esp_err_t app_matter_init(esp_matter::attribute::callback_t update_cb,
                          esp_matter::identification::callback_t identification_cb);
esp_err_t app_matter_light_create(app_driver_handle_t driver_handle);
esp_err_t app_matter_start();

void app_matter_rainmaker_device_create(esp_rmaker_device_write_cb_t write_cb);
esp_err_t app_matter_pre_rainmaker_start();
