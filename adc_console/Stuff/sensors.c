#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "sensors.h"

#define SOIL_PWR_PIN 14
#define PHOTO_ADC_CH 2
#define DHT_PIN 15

float read_soil(uint8_t channel) {
    const float conversion_factor = 3.3f / (1 << 12);
    adc_select_input(channel);
    uint16_t value = adc_read();
    printf("Soil (CH%d): %u (%.2f V)\n", channel, value, value * conversion_factor);
    return value * conversion_factor;
}

float read_photo(void) {
    const float conversion_factor = 3.3f / (1 << 12);
    adc_select_input(PHOTO_ADC_CH);
    uint16_t value = adc_read();
    printf("Photo: %u (%.2f V)\n", value, value * conversion_factor);
    return value * conversion_factor;
}

int read_dht11(float *humidity, float *temperature) {
    uint8_t data[5] = {0};
    uint32_t start_time;

    gpio_init(DHT_PIN);
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(20);
    gpio_put(DHT_PIN, 1);
    sleep_us(30);
    gpio_set_dir(DHT_PIN, GPIO_IN);

    start_time = time_us_32();
    while (gpio_get(DHT_PIN) == 1) if (time_us_32() - start_time > 100) return 1;
    start_time = time_us_32();
    while (gpio_get(DHT_PIN) == 0) if (time_us_32() - start_time > 100) return 2;
    start_time = time_us_32();
    while (gpio_get(DHT_PIN) == 1) if (time_us_32() - start_time > 100) return 3;

    for (int i = 0; i < 40; ++i) {
        start_time = time_us_32();
        while (gpio_get(DHT_PIN) == 0) if (time_us_32() - start_time > 70) return 4;
        start_time = time_us_32();
        while (gpio_get(DHT_PIN) == 1) if (time_us_32() - start_time > 100) break;
        uint32_t pulse = time_us_32() - start_time;
        data[i/8] <<= 1;
        if (pulse > 40) data[i/8] |= 1;
    }

    if (((uint8_t)(data[0] + data[1] + data[2] + data[3])) != data[4]) return 5;

    *humidity = data[0];
    *temperature = data[2];
    return 0;
}

void soil_power_on(void) {
    gpio_put(SOIL_PWR_PIN, 1);
}

void soil_power_off(void) {
    gpio_put(SOIL_PWR_PIN, 0);
}