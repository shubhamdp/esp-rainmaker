/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include <stdbool.h>

#define DEFAULT_POWER  true

void app_driver_init(void);

bool app_driver_get_light_state(void);
bool app_driver_get_fan_state(void);
bool app_driver_get_cooler_state(void);

void app_driver_set_light_state(bool state);
void app_driver_set_fan_state(bool state);
void app_driver_set_cooler_state(bool state);

void app_diag_init(void);
