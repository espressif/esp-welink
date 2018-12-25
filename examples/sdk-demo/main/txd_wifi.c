/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS chips only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "esp_welink_log.h"
#include "txd_welink.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define TEST_WIFI_SSID         CONFIG_WELINK_WiFi_SSID
#define TEST_WIFI_PASSWORD     CONFIG_WELINK_WiFi_PASSWORD

static const char* TAG = "txd_wifi";
static EventGroupHandle_t wifi_event_group;
extern xQueueHandle welink_task_queue;

static void wifi_connection(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = TEST_WIFI_SSID,
            .password = TEST_WIFI_PASSWORD,
        },
    };
    WELINK_LOGI("Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    esp_wifi_connect();
}

static esp_err_t event_handler(void* ctx, system_event_t* event)
{
    ESP_LOGI("TEST", "event = %d", event->event_id);

    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            WELINK_LOGI("SYSTEM_EVENT_STA_START");
            wifi_connection();
            break;

        case SYSTEM_EVENT_SCAN_DONE:
            WELINK_LOGI("SYSTEM_EVENT_SCAN_DONE");
            break;

        case SYSTEM_EVENT_STA_GOT_IP:
            WELINK_LOGI("Got IPv4[%s]", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

            uint8_t msg = 0;

            if (xQueueSend(welink_task_queue, &msg, 10 / portTICK_RATE_MS) != pdTRUE) {
                WELINK_LOGE("%s xQueue send failed", __func__);
            }

            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP WiFi libs don't currently auto-reassociate. */
            WELINK_LOGI("SYSTEM_EVENT_STA_DISCONNECTED");
            esp_wifi_connect();
            break;

        default:
            break;
    }

    return ESP_OK;
}

void esp_wifi_handler(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}


