#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/apps/http_client.h"
#include "lwip/dns.h"

#include "lwip/apps/http_client.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include <string.h>
#include <stdio.h>

#include <stdlib.h>
#include <time.h>

// pola w thingspeak (fields)
// 1 - temperatura
// 2 - wilgotność powietrza
// 3 - wilgotność gleby
// 4 - natężenie światła
// 5 - poziom c02
// 6 - pH gleby


#define LED_DELAY_MS 250
#define WIFI_SSID "TP-Link_BDFE"
#define WIFI_PASS "82824044"
#define THINGSPEAK_API_KEY_WRITE "Y3UPR45H87QGLL89"
#define THINGSPEAK_API_KEY_READ "D5MF670CBBL3ODH0"

int pico_led_init(void) {
    return cyw43_arch_init();
}

void pico_set_led(bool led_on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
}

static void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
    printf("HTTP result: %d, len: %lu, server res: %lu, err: %d\n", httpc_result, rx_content_len, srv_res, err);
}

static err_t thingspeak_headers_done(httpc_state_t *conn, void *arg, struct pbuf *hdr, u16_t hdr_len,u32_t content_len) 
{
    pbuf_free(hdr);
    return ERR_OK;
}

static err_t thingspeak_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) 
{
    if (p == NULL) {
        return ERR_OK;
    }
    tcp_recved(pcb, p->tot_len);
    return ERR_OK;
}

int random_range(int min, int max) {
    if (max <= min) return min;
    return min + rand() % (max - min + 1);
}

void send_to_thingspeak(float data, int field) 
{
    char uri[128];
    snprintf(uri, sizeof(uri), "/update?api_key=%s&field%d=%.2f", THINGSPEAK_API_KEY_WRITE, field, data);

    httpc_connection_t settings = {
        .use_proxy        = 0,
        .headers_done_fn  = thingspeak_headers_done,
        .result_fn        = http_result,
    };

    httpc_state_t *connection = NULL;

    err_t err = httpc_get_file_dns(
        "api.thingspeak.com",  // host
        80,                    // port
        uri,                   // path
        &settings,
        thingspeak_recv,       // body callback
        NULL,                  // callback_arg (unused)
        &connection            // get back the state handle
    );

    if (err != ERR_OK) {
        printf("httpc_get_file_dns failed: %d\n", err);
    }

    // ThingSpeak rate limit: 15s between updates
    sleep_ms(16000);
}




int main() {
    stdio_init_all();

    // Inicjalizacja sterownika Wi-Fi
    if (cyw43_arch_init()) {
        printf("Błąd inicjalizacji cyw43_arch\n");
        return 1;
    }

    // Włącz tryb stacji (klienta Wi-Fi)
    cyw43_arch_enable_sta_mode();

    printf("Łączenie z siecią Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS,
            CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Nie udało się połączyć z siecią Wi-Fi\n");
        return 1;
    }

    printf("Połączono z siecią Wi-Fi!\n");

    const ip_addr_t *dns = dns_getserver(0);
    printf("DNS: %s\n", ipaddr_ntoa(dns));



    // pętla główna
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(500);



        send_to_thingspeak(random_range(18, 25), 1); // przykładowa temperatura
        send_to_thingspeak(random_range(0, 100), 2); // przykładowa wilgotność powietrza
        printf("Wysyłanie danych do ThingSpeak...\n");
    }
}
