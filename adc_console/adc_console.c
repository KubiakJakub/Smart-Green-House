#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "sensors.h"           // Obsługa czujników: gleba, światło, DHT11
#include "motors.h"            // Obsługa silników i wentylatora
#include "system_status.h"     // Struktura przechowująca stan systemu
#include "pico/cyw43_arch.h"   // Obsługa Wi-Fi 
#include "hcsr04.h"            // Obsługa czujnika ultradźwiękowego HC-SR04
#include "wifi_thingspeak.h"   // Funkcja wysyłająca dane do ThingSpeak
#include "secrets.h"

#define SOIL_ADC_CH_COUNT 2
const uint8_t SOIL_ADC_CH[SOIL_ADC_CH_COUNT] = {0, 1}; // Kanały ADC dla czujników gleby
const uint32_t measurement_interval_ms = 1800000;        // Interwał pełnego pomiaru (ms) - 30 minut
//const uint32_t measurement_interval_ms = 60000;        // Interwał pełnego pomiaru (ms) - 1 minuta
const uint32_t motor_update_interval_ms = 100;         // Interwał aktualizacji silników (ms)

#define HCSR04_TRIG_PIN 20     // Pin TRIG dla HC-SR04
#define HCSR04_ECHO_PIN 21     // Pin ECHO dla HC-SR04

volatile int8_t fan_speed = 0;    // Prędkość wentylatora (aktualna)
volatile int8_t motor_speed = 0;  // Prędkość pompy (aktualna)

uint32_t last_measurement_time = 0; // Czas ostatniego pełnego pomiaru
uint32_t last_motor_update = 0;     // Czas ostatniej aktualizacji silników

int main() {
    // Inicjalizacja peryferiów i GPIO
    stdio_init_all();
    adc_init();
    gpio_init(14);                // Zasilanie czujnika gleby
    gpio_set_dir(14, GPIO_OUT);
    gpio_put(14, 0);

    motors_init();                // Inicjalizacja silników
    hcsr04_init(HCSR04_TRIG_PIN, HCSR04_ECHO_PIN); // Inicjalizacja HC-SR04

    printf("\n===========================\n");
    printf("RP2350 Analog Sensors + DHT11\n");
    printf("===========================\n");

    // Ustawienie początkowych czasów
    last_measurement_time = to_ms_since_boot(get_absolute_time());
    last_motor_update = last_measurement_time;

    float last_humidity = 0.0f;      // Ostatni odczyt wilgotności powietrza
    float last_soil_voltage = 0.0f;  // Ostatni odczyt wilgotności gleby
    bool pump_on = false;            // Stan pompy
    uint32_t soil_measurement_interval = measurement_interval_ms; // Interwał pomiaru gleby

    SystemStatus status = {0};       // Struktura przechowująca aktualny stan systemu

    // Inicjalizacja Wi-Fi
    if (cyw43_arch_init()) {
        printf("Błąd inicjalizacji cyw43_arch\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    printf("Łączenie z siecią Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Nie udało się połączyć z siecią Wi-Fi\n");
        return 1;
    }
    printf("Połączono z siecią Wi-Fi!\n");

    // Główna pętla programu
    while (1) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // --- Sterowanie wentylatorem i pompą co motor_update_interval_ms ---
        if (now - last_motor_update >= motor_update_interval_ms) {
            // Wentylator: włącz jeśli wilgotność powietrza > 60%
            if (last_humidity > 60.0f) {
                fan_12v_set_speed(100);
                status.fan_speed = 100;
                status.fan_on = true;
            } else {
                fan_12v_set_speed(0);
                status.fan_speed = 0;
                status.fan_on = false;
            }
            // Pompa: włącz/wyłącz zgodnie z flagą pump_on
            if (pump_on) {
                motor_5v_set_speed(70);
                status.pump_speed = 70;
                status.pump_on = true;
            } else {
                motor_5v_set_speed(0);
                status.pump_speed = 0;
                status.pump_on = false;
            }
            last_motor_update = now;
        }

        // --- Pełny cykl pomiarowy co soil_measurement_interval ---
        if (now - last_measurement_time >= soil_measurement_interval) {
            soil_power_on();         // Włącz zasilanie czujnika gleby
            sleep_ms(500);           // Poczekaj na stabilizację

            // Odczyt wilgotności gleby (ADC)
            last_soil_voltage = read_soil(SOIL_ADC_CH[0]);
            status.soil_voltage = last_soil_voltage;

            soil_power_off();        // Wyłącz zasilanie czujnika gleby

            printf("Soil voltage: %.3f V\n", last_soil_voltage);

            // Logika sterowania pompą na podstawie wilgotności gleby
            if (last_soil_voltage >= 2.1f) {
                printf("Stan gleby: SUCHO -> POMPA ON\n");
                pump_on = true;
                soil_measurement_interval = 1000; // Szybsze sprawdzanie podczas podlewania
            } else if (last_soil_voltage < 2.1f && last_soil_voltage >= 1.4f) {
                printf("Stan gleby: NAWILŻONY/PRZEMOCZONY -> POMPA OFF\n");
                pump_on = false;
                soil_measurement_interval = measurement_interval_ms; // Powrót do normalnego interwału
            } else {
                printf("Stan gleby: POZA ZAKRESEM (%.3f V)\n", last_soil_voltage);
                pump_on = true;
            }

            // --- Pełny cykl pomiarowy: odczyt pozostałych czujników i wysyłka ---
            if (soil_measurement_interval == measurement_interval_ms) {
                // Odczyt natężenia światła (ADC)
                status.photo_voltage = read_photo();

                // Odczyt temperatury i wilgotności powietrza (DHT11)
                float humidity = 0.0f, temperature = 0.0f;
                int result = read_dht11(&humidity, &temperature);
                if (result == 0) {
                    printf("DHT11: Temp: %.1f C, Humidity: %.1f %%\n", temperature, humidity);
                    last_humidity = humidity;
                    status.humidity = humidity;
                    status.temperature = temperature;
                } else {
                    printf("DHT11: Read error! (%d)\n", result);
                }

                // Odczyt poziomu wody (HC-SR04)
                float water_level_cm = hcsr04_get_distance_cm(HCSR04_TRIG_PIN, HCSR04_ECHO_PIN);
                printf("Poziom wody: %.1f cm\n", water_level_cm);
                status.water_level = water_level_cm;

                if (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP) {
                    printf("Wi-Fi rozłączone, próbuję ponownie połączyć...\n");
                    cyw43_arch_deinit();
                    sleep_ms(1000);
                    cyw43_arch_init();
                    cyw43_arch_enable_sta_mode();
                    cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000);
                }
                // Wysyłanie wszystkich danych do ThingSpeak
                send_status_to_thingspeak(&status);
            }

            last_measurement_time = now;
        }

        sleep_ms(10); // Krótka pauza, by nie obciążać CPU
    }
}