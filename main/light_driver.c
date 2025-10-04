/*
 * SPDX-FileCopyrightText: 2025 MounMovies
 *
 */

#include "esp_log.h"
#include "light_driver.h"
#include <driver/rtc_io.h>
#include "esp_sleep.h"
#include "driver/ledc.h"
#if CONFIG_HALLOWEEN_BLINK_ENABLE
#include "esp_timer.h"
#endif

#define MM_LED_GPIO		4
#define MM_LED_LEDC_CH 	1

#define BLINK_TIME_ON_MS   1100
#define BLINK_TIME_OFF_MS  800

static const char *TAG = "LED";
static bool mm_light_initialized = false;
#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
static bool mm_brightness_started = false;
static uint8_t mm_brightness_last = LIGHT_DEFAULT_BRIGHTNESS;

void light_brightness_init(void);
void light_brightness_start(void);
void light_brightness_stop(void);
#endif //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE

#if CONFIG_HALLOWEEN_BLINK_ENABLE
void light_effect_init(void);
void start_effect(void);
void stop_effect(void);
#endif //HALLOWEEN_BLINK_ENABLE

void led_rtc_init(void);
static void led_set_power(bool power);

void light_driver_set_power(bool power)
{
#if CONFIG_HALLOWEEN_BLINK_ENABLE
	if (power)
		start_effect();
	else
		stop_effect();
#else //CONFIG_HALLOWEEN_BLINK_ENABLE
    led_set_power(power);
#endif //CONFIG_HALLOWEEN_BLINK_ENABLE

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
	ESP_LOGI(TAG, "Initialized LED with brightness.");
#else //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
	led_rtc_init();
	ESP_LOGI(TAG, "Initialized LED with RTC.");
#endif //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
	
#if CONFIG_HALLOWEEN_BLINK_ENABLE
	light_effect_init();
	ESP_LOGI(TAG, "Initialized LED blink effect.");
#endif //HALLOWEEN_BLINK_ENABLE

    mm_light_initialized = true;
	light_driver_set_power(power);
}

void led_rtc_init(void)
{
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_ON);
	
    rtc_gpio_init(MM_LED_GPIO);
    rtc_gpio_set_direction(MM_LED_GPIO, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_pulldown_dis(MM_LED_GPIO);
    rtc_gpio_pullup_dis(MM_LED_GPIO);
}

void led_rtc_power(bool power)
{
	rtc_gpio_hold_dis(MM_LED_GPIO);
#if CONFIG_HALLOWEEN_LED_LEVEL_HIGH
	rtc_gpio_set_level(MM_LED_GPIO, power);
#else
	rtc_gpio_set_level(MM_LED_GPIO, !power);
#endif
    rtc_gpio_hold_en(MM_LED_GPIO);
}

static void led_set_power(bool power)
{
#if CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
    if (power)
    {
        light_driver_set_brightness(mm_brightness_last);
    }
    else
    {
		light_brightness_stop();
    }
#else //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
    led_rtc_power(power);
#endif //CONFIG_HALLOWEEN_BRIGHTNESS_ENABLE
}

#if CONFIG_HALLOWEEN_BLINK_ENABLE
static esp_timer_handle_t blink_timer;
static bool blink_state = false;
static bool blink_en = false;

static void effect_timer_callback(void* arg) {
    if (!blink_en) return;

    blink_state = !blink_state;

    if (blink_state) {
		led_set_power(1);
        // Set timer to ON_TIME
        esp_timer_stop(blink_timer);
        esp_timer_start_once(blink_timer, BLINK_TIME_ON_MS * 1000ULL);
    } else {
		led_set_power(0);
        // Set timer to OFF_TIME
        esp_timer_stop(blink_timer);
        esp_timer_start_once(blink_timer, BLINK_TIME_OFF_MS * 1000ULL);
    }
}

void light_effect_init(void)
{			
	/* timer for efekt */
    const esp_timer_create_args_t timer_args = {
        .callback = &effect_timer_callback,
        .name = "blink_timer"
    };
    esp_timer_create(&timer_args, &blink_timer);
}

void start_effect(void) 
{
    if (blink_en) return;
    blink_en = true;
    blink_state = false;
    esp_timer_start_once(blink_timer, 1000);
}

void stop_effect(void) 
{
    blink_en = false;
    esp_timer_stop(blink_timer);
	led_set_power(0);
}
#endif

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
        .freq_hz = (CONFIG_HALLOWEEN_BRIGHTNESS_FREQ),
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
