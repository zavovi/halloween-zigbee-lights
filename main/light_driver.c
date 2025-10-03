/*
 * SPDX-FileCopyrightText: 2025 MounMovies
 *
 */

#include "esp_log.h"
#include "light_driver.h"
#include <driver/rtc_io.h>
#include "esp_sleep.h"
#include "driver/ledc.h"
#if CONFIG_HALLOWEEN_LIGHT_EFFECTS
#include "led_indicator.h"
#include "led_indicator_gpio.h"
#endif

#define MM_LED_GPIO	4
#define MM_LED_LEDC_CH 1

static const char *TAG = "LED";
static bool mm_light_initialized = false;
#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
static bool mm_brightness_started = false;
static uint8_t mm_brightness_last = LIGHT_DEFAULT_BRIGHTNESS;

void light_brightness_init(void);
void light_brightness_start(void);
void light_brightness_stop(void);
#endif //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE


#if CONFIG_HALLOWEEN_LIGHT_EFFECTS
static const blink_step_t bsp_led_on[] = {
#if CONFIG_HALLOWEEN_LED_LEVEL_HIGH
    {LED_BLINK_HOLD, LED_STATE_ON, 1000},
#else
    {LED_BLINK_HOLD, LED_STATE_OFF, 1000},
#endif
    {LED_BLINK_STOP, 0, 0},
};
static const blink_step_t bsp_led_off[] = {
#if CONFIG_HALLOWEEN_LED_LEVEL_HIGH
    {LED_BLINK_HOLD, LED_STATE_OFF, 1000},
#else
    {LED_BLINK_HOLD, LED_STATE_ON, 1000},
#endif
    {LED_BLINK_STOP, 0, 0},
};
static const blink_step_t bsp_led_blink[] = {
#if CONFIG_HALLOWEEN_LED_LEVEL_HIGH
    {LED_BLINK_HOLD, LED_STATE_ON, 1100},
    {LED_BLINK_HOLD, LED_STATE_OFF, 800},
#else
    {LED_BLINK_HOLD, LED_STATE_OFF, 1100},
    {LED_BLINK_HOLD, LED_STATE_ON, 800},
#endif
    {LED_BLINK_LOOP, 0, 0},
};
enum {
    BSP_LED_ON,
    BSP_LED_OFF,
    BSP_LED_BLINK,
    BSP_LED_MAX,
};
blink_step_t const *bsp_led_blink_defaults_lists[] = {
    [BSP_LED_ON] = bsp_led_on,
    [BSP_LED_OFF] = bsp_led_off,
    [BSP_LED_BLINK] = bsp_led_blink,
    [BSP_LED_MAX] = NULL,
};
static led_indicator_handle_t mm_led_handle = NULL;
#endif //CONFIG_HALLOWEEN_LIGHT_EFFECTS

void light_driver_set_power(bool power)
{
#if CONFIG_HALLOWEEN_LIGHT_EFFECTS
	if (power)
	{
		led_indicator_stop(mm_led_handle, BSP_LED_OFF);
		led_indicator_start(mm_led_handle, BSP_LED_BLINK);
	}
	else
	{
		led_indicator_stop(mm_led_handle, BSP_LED_BLINK);
		led_indicator_start(mm_led_handle, BSP_LED_OFF);
	}
#else //CONFIG_HALLOWEEN_LIGHT_EFFECTS

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
#else //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
    rtc_gpio_hold_dis(MM_LED_GPIO);
#if CONFIG_HALLOWEEN_LED_LEVEL_HIGH
	rtc_gpio_set_level(MM_LED_GPIO, power);
#else
	rtc_gpio_set_level(MM_LED_GPIO, !power);
#endif
    rtc_gpio_hold_en(MM_LED_GPIO);
#endif //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE

#endif //CONFIG_HALLOWEEN_LIGHT_EFFECTS

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
	
#if CONFIG_HALLOWEEN_LIGHT_EFFECTS
	const led_indicator_gpio_config_t mm_led_gpio_config = {
		.is_active_level_high = CONFIG_HALLOWEEN_LED_LEVEL_HIGH,
		.gpio_num = MM_LED_GPIO,
	};
	const led_indicator_config_t mm_led_config = {
		.blink_lists = bsp_led_blink_defaults_lists,
		.blink_list_num = BSP_LED_MAX,
	};
	led_indicator_new_gpio_device(&mm_led_config, &mm_led_gpio_config, &mm_led_handle);
	ESP_LOGI(TAG, "Initialized LED with effects.");
#else //CONFIG_HALLOWEEN_LIGHT_EFFECTS
	
#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE 
	light_brightness_init();
    light_driver_set_brightness(mm_brightness_last);
	ESP_LOGI(TAG, "Initialized LED with brightness.");
#else //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
	
    rtc_gpio_init(MM_LED_GPIO);
    rtc_gpio_set_direction(MM_LED_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_pulldown_dis(MM_LED_GPIO);
    rtc_gpio_pullup_dis(MM_LED_GPIO);
	ESP_LOGI(TAG, "Initialized LED with power-saving.");
#endif //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE

#endif //CONFIG_HALLOWEEN_LIGHT_EFFECTS

    mm_light_initialized = true;
	light_driver_set_power(power);
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
