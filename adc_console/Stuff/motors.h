#ifndef MOTORS_H
#define MOTORS_H

#include <stdint.h>

void motors_init(void);
void fan_12v_set_speed(int8_t speed);
void motor_5v_set_speed(int8_t speed);

#endif // MOTORS_H