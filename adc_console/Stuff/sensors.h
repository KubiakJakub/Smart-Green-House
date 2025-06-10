#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

float read_soil(uint8_t channel);
float read_photo(void);
int read_dht11(float *humidity, float *temperature);
void soil_power_on(void);
void soil_power_off(void);

#endif // SENSORS_H