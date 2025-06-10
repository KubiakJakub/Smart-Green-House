#include <stdlib.h>
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "motors.h"

#define M1A_PIN 16
#define M1B_PIN 17
#define M2A_PIN 18
#define M2B_PIN 19

void motors_init(void) {
    gpio_set_function(M1A_PIN, GPIO_FUNC_PWM);
    gpio_set_function(M1B_PIN, GPIO_FUNC_PWM);
    gpio_set_function(M2A_PIN, GPIO_FUNC_PWM);
    gpio_set_function(M2B_PIN, GPIO_FUNC_PWM);

    uint slice_m1a = pwm_gpio_to_slice_num(M1A_PIN);
    uint slice_m1b = pwm_gpio_to_slice_num(M1B_PIN);
    uint slice_m2a = pwm_gpio_to_slice_num(M2A_PIN);
    uint slice_m2b = pwm_gpio_to_slice_num(M2B_PIN);

    pwm_set_wrap(slice_m1a, 255);
    pwm_set_wrap(slice_m1b, 255);
    pwm_set_wrap(slice_m2a, 255);
    pwm_set_wrap(slice_m2b, 255);

    pwm_set_enabled(slice_m1a, true);
    pwm_set_enabled(slice_m1b, true);
    pwm_set_enabled(slice_m2a, true);
    pwm_set_enabled(slice_m2b, true);
}

void fan_12v_set_speed(int8_t speed) {
    if (speed > 0) {
        pwm_set_gpio_level(M1A_PIN, speed * 255 / 100);
        pwm_set_gpio_level(M1B_PIN, 0);
    } else if (speed < 0) {
        pwm_set_gpio_level(M1A_PIN, 0);
        pwm_set_gpio_level(M1B_PIN, -speed * 255 / 100);
    } else {
        pwm_set_gpio_level(M1A_PIN, 0);
        pwm_set_gpio_level(M1B_PIN, 0);
    }
}

void motor_5v_set_speed(int8_t speed) {
    // Ogranicz PWM tak, by napięcie na silniku nie przekraczało 5V przy zasilaniu 12V
    float max_pwm = 255.0f * 5.0f / 12.0f; // ≈ 106
    int pwm_val = (int)(abs(speed) * max_pwm / 100.0f);

    if (speed > 0) {
        pwm_set_gpio_level(M2A_PIN, pwm_val);
        pwm_set_gpio_level(M2B_PIN, 0);
    } else if (speed < 0) {
        pwm_set_gpio_level(M2A_PIN, 0);
        pwm_set_gpio_level(M2B_PIN, pwm_val);
    } else {
        pwm_set_gpio_level(M2A_PIN, 0);
        pwm_set_gpio_level(M2B_PIN, 0);
    }
}