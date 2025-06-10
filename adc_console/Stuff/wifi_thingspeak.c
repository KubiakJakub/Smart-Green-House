#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "wifi_thingspeak.h"

#define THINGSPEAK_API_KEY_WRITE "IRKGLMH9RTX5J0IR"

static void http_result(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    printf("HTTP result: %d, len: %lu, server res: %lu, err: %d\n", httpc_result, rx_content_len, srv_res, err);
}

static err_t thingspeak_headers_done(httpc_state_t *conn, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len) {
    // NIE wywołuj pbuf_free(hdr)!
    return ERR_OK;
}

static err_t thingspeak_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        return ERR_OK;
    }
    tcp_recved(pcb, p->tot_len);
    return ERR_OK;
}

void send_status_to_thingspeak(const SystemStatus* status) {
    char uri[512];
    snprintf(uri, sizeof(uri),
        "/update?api_key=%s"
        "&field1=%.2f"   // soil_voltage
        "&field2=%.2f"   // photo_voltage
        "&field3=%.2f"   // humidity
        "&field4=%.2f"   // temperature
        // field5 pomijamy
        "&field6=%d"     // pump_on
        "&field7=%d"     // fan_on
        "&field8=%.2f",  // water_level
        THINGSPEAK_API_KEY_WRITE,
        status->soil_voltage,    // field1
        status->photo_voltage,   // field2
        status->humidity,        // field3
        status->temperature,     // field4
        status->pump_on ? 1 : 0, // field6
        status->fan_on ? 1 : 0,  // field7
        status->water_level      // field8
    );

    httpc_connection_t settings = {
        .use_proxy        = 0,
        .headers_done_fn  = thingspeak_headers_done,
        .result_fn        = http_result,
    };

    httpc_state_t *connection = NULL;

    err_t err = httpc_get_file_dns(
        "api.thingspeak.com",
        80,
        uri,
        &settings,
        thingspeak_recv,
        NULL,
        &connection
    );

    if (err != ERR_OK) {
        printf("httpc_get_file_dns failed: %d\n", err);
    }

    sleep_ms(16000);
}