#ifndef WIFI_THINGSPEAK_H
#define WIFI_THINGSPEAK_H

#include "system_status.h"

void send_status_to_thingspeak(const SystemStatus* status);

#endif // WIFI_THINGSPEAK_H