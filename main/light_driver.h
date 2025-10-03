/*
 * SPDX-FileCopyrightText: 2025 MounMovies
 *
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* light intensity level */
#define LIGHT_DEFAULT_ON  1
#define LIGHT_DEFAULT_OFF 0

#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
#define LIGHT_DEFAULT_BRIGHTNESS 125
#endif

/**
* @brief Set light power (on/off).
*
* @param  power  The light power to be set
*/
void light_driver_set_power(bool power);

/**
* @brief Set light brightness (0 - 100%).
*
* @param  value  The light brightness value to be set
*/
void light_driver_set_brightness(uint8_t value);

/**
* @brief color light driver init, be invoked where you want to use color light
*
* @param power power on/off
*/
void light_driver_init(bool power);

#ifdef __cplusplus
} // extern "C"
#endif
