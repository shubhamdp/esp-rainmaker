/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include <esp_err.h>
#include <esp_matter.h>
#include <esp_matter_attribute_utils.h>
#include <esp_rmaker_core.h>

esp_matter_attr_val_t app_rainmaker_get_matter_val(esp_rmaker_param_val_t *val, uint32_t cluster_id,
                                                          uint32_t attribute_id);
esp_rmaker_param_val_t app_rainmaker_get_rmaker_val(esp_matter_attr_val_t *val, uint32_t cluster_id,
                                                           uint32_t attribute_id);
const char *app_rainmaker_get_device_name_from_id(uint32_t endpoint_id);
const char *app_rainmaker_get_param_name_from_id(uint32_t cluster_id, uint32_t attribute_id);
uint16_t app_rainmaker_get_endpoint_id_from_name(const char *device_name);
uint32_t app_rainmaker_get_cluster_id_from_name(const char *param_name);
uint32_t app_rainmaker_get_attribute_id_from_name(const char *param_name);
