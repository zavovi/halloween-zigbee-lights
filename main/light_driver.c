/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: LicenseRef-Included
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Espressif Systems
 *    integrated circuit in a product or a software update for such product,
 *    must reproduce the above copyright notice, this list of conditions and
 *    the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "esp_log.h"
#include "driver/gpio.h"
#include "light_driver.h"
#include <driver/rtc_io.h>
#include "esp_sleep.h"
#include "driver/ledc.h"

#define MM_LED_GPIO		GPIO_NUM_4
#define MM_LED_LEDC_CH 	1

#define MM_RELAY_GPIO_1	GPIO_NUM_10
#define MM_RELAY_GPIO_2	GPIO_NUM_11
#define MM_RELAY_GPIO_3	GPIO_NUM_22
#define MM_RELAY_GPIO_4	GPIO_NUM_23

static bool mm_light_initialized = false;
#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
static bool mm_brightness_started = false;
static uint8_t mm_brightness_last = LIGHT_DEFAULT_BRIGHTNESS;

void light_brightness_init(void);
void light_brightness_start(void);
void light_brightness_stop(void);
#endif

void light_driver_set_power(bool power)
{
#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
    if (power)
    {
        if (mm_brightness_last < 10)
            mm_brightness_last = 255;
        light_driver_set_brightness(mm_brightness_last);
    }
    else
    {
        light_driver_set_brightness(0);
		light_brightness_stop();
    }
#else
    rtc_gpio_hold_dis(MM_LED_GPIO);
#if CONFIG_HALLOWEEN_LED_LEVEL_HIGH
	rtc_gpio_set_level(MM_LED_GPIO, power);
#else
	rtc_gpio_set_level(MM_LED_GPIO, !power);
#endif
    rtc_gpio_hold_en(MM_LED_GPIO);
#endif //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
}

void light_driver_set_brightness(uint8_t value)
{
#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
	if (value > 0 && !mm_brightness_started)
		light_brightness_start();
	
    mm_brightness_last = value;

#if CONFIG_HALLOWEEN_LED_LEVEL_HIGH
    uint32_t duty_cycle = (1023 * value) / 255; // LEDC resolution set to 10bits, thus: 100% = 1023
#else
    uint32_t duty_cycle = (1023 * (255-value)) / 255; // LEDC resolution set to 10bits, thus: 100% = 1023
#endif
    ledc_set_duty(LEDC_LOW_SPEED_MODE, MM_LED_LEDC_CH, duty_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, MM_LED_LEDC_CH);
	
	if (value == 0 && mm_brightness_started)
		light_brightness_stop();
	
#endif //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
}

void light_driver_init(bool power)
{
    if(mm_light_initialized)
        return;
#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE 
	light_brightness_init();
    light_driver_set_brightness(mm_brightness_last);
#else
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
	
    rtc_gpio_init(MM_LED_GPIO);
    rtc_gpio_set_direction(MM_LED_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_pulldown_dis(MM_LED_GPIO);
    rtc_gpio_pullup_dis(MM_LED_GPIO);
#endif //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
    mm_light_initialized = true;
	light_driver_set_power(power);
}

void relay_driver_init(void)
{
	const gpio_config_t io_config = {
		.pin_bit_mask = BIT64(MM_RELAY_GPIO_1) | BIT64(MM_RELAY_GPIO_2) | BIT64(MM_RELAY_GPIO_3) | BIT64(MM_RELAY_GPIO_4),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
	};
	ESP_ERROR_CHECK(gpio_config(&io_config));
}

void relay_driver_set_power(uint8_t number, bool enabled)
{
	switch(number)
	{
	case 1:
		gpio_set_level(MM_RELAY_GPIO_1, enabled);
		break;
	case 2:
		gpio_set_level(MM_RELAY_GPIO_2, enabled);
		break;
	case 3:
		gpio_set_level(MM_RELAY_GPIO_3, enabled);
		break;
	case 4:
		gpio_set_level(MM_RELAY_GPIO_4, enabled);
		break;
	}
}

#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
void light_brightness_init(void)
{	
    // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = MM_LED_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = MM_LED_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = 1,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE
    };
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = 1,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ledc_timer_config(&LCD_backlight_timer);
    ledc_channel_config(&LCD_backlight_channel);
	   
	mm_brightness_started = true;
}

void light_brightness_start(void)
{
    ledc_timer_resume(LEDC_LOW_SPEED_MODE, 1);
	
	mm_brightness_started = true;
}

void light_brightness_stop(void)
{
	ledc_stop(LEDC_LOW_SPEED_MODE, MM_LED_LEDC_CH, 0);
	ledc_timer_pause(LEDC_LOW_SPEED_MODE, 1);
	
	mm_brightness_started = false;
}
#endif
