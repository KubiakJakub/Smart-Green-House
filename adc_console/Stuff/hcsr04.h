#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

void hcsr04_init(uint trig_pin, uint echo_pin);
float hcsr04_get_distance_cm(uint trig_pin, uint echo_pin);

#endif // HCSR04_H