#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float soil_voltage;
    float photo_voltage;
    float humidity;
    float temperature;
    int8_t fan_speed;
    int8_t pump_speed;
    bool pump_on;
    bool fan_on;
    float water_level;
} SystemStatus;

#endif // SYSTEM_STATUS_H