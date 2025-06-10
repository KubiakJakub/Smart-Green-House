#include "pico/stdlib.h"
#include "hcsr04.h"

void hcsr04_init(uint trig_pin, uint echo_pin) {
    gpio_init(trig_pin);
    gpio_set_dir(trig_pin, GPIO_OUT);
    gpio_put(trig_pin, 0);

    gpio_init(echo_pin);
    gpio_set_dir(echo_pin, GPIO_IN);
}

float hcsr04_get_distance_cm(uint trig_pin, uint echo_pin) {
    // Trigger pulse
    gpio_put(trig_pin, 1);
    sleep_us(10);
    gpio_put(trig_pin, 0);

    // Wait for echo start
    absolute_time_t start = get_absolute_time();
    while (gpio_get(echo_pin) == 0) {
        if (absolute_time_diff_us(start, get_absolute_time()) > 30000) return -1.0f; // timeout
    }
    absolute_time_t echo_start = get_absolute_time();

    // Wait for echo end
    while (gpio_get(echo_pin) == 1) {
        if (absolute_time_diff_us(echo_start, get_absolute_time()) > 30000) return -1.0f; // timeout
    }
    absolute_time_t echo_end = get_absolute_time();

    int64_t pulse_us = absolute_time_diff_us(echo_start, echo_end);
    float distance_cm = pulse_us / 58.0f; // 58us per cm (speed of sound)
    return distance_cm;
}