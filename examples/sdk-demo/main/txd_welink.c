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

#include <sys/socket.h>
#include <sys/errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_ota_ops.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_welink_log.h"
#include "netdb.h"
#include "lwip/inet.h"

#include "txd_sdk.h"
#include "txd_stdapi.h"
#include "txd_baseapi.h"
#include "txd_error.h"
#include "txd_ota.h"
#include "txd_stdtypes.h"
#include "txd_thread.h"

static const char* TAG = "txd_welink";

#define TEST_SN        CONFIG_WELINK_SN
#define TEST_LICENSE   CONFIG_WELINK_LICENSE
#define TEST_PID       CONFIG_WELINK_PID

static const uint8_t fw_version[]     = "v1.0.0";
static const uint8_t CLIENT_PUB_KEY[] = {0x02, 0x63, 0x16, 0xD4, 0xE3, 0x7B, 0xFE, 0x2B, 0xD1, 0x72, 0x99, 0xAF, 0x86, 0x26, 0xC2, 0xF1, 0xCC, 0x50, 0xF4, 0xCF, 0x3E, 0x54, 0x58, 0x5D, 0x08};
static const uint8_t AUTH_KEY[]       = {0x1F, 0xBF, 0xE8, 0x30, 0xA6, 0x94, 0xA3, 0xCB, 0xEA, 0xE8, 0xB8, 0xF0, 0x28, 0xD4, 0x1C, 0x9C};
static uint8_t g_ota_url[512] = {0};

extern xQueueHandle welink_task_queue;

/*****************************OTA************************************/
xQueueHandle welink_ota_queue = NULL;

typedef struct {
    uint32_t status_code;
    uint8_t  *body;
    uint32_t body_len;
} http_response_result_t;

static int32_t txd_http_download(const uint8_t *url, const uint8_t *file_type, const uint8_t *file_key);
static void get_host_from_url(const uint8_t *url, uint8_t *host, uint32_t host_size, uint8_t *path);
static bool parse_http_response(const uint8_t *response, uint32_t response_len, http_response_result_t *result);

void get_host_from_url(const uint8_t *url, uint8_t *host, uint32_t host_size, uint8_t *path) {
    uint32_t i, url_len, s, t;
    url_len = txd_strlen((char *)url);

    s = 0;
    for (i = 0; i < url_len; ++i) {
        if (url[i] == '/') {
            s = i + 2;
            break;
        }
    }

    t = url_len;
    for (i = s; i < url_len; ++i) {
        if (url[i] == '/') {
            t = i;
            break;
        }
    }

    // host: url[s,t)
    if (s && t && (t-s) < host_size) {
        txd_memcpy(host, url + s, t - s);
        host[t-s] = '\0';
    }

    // path: ulr[t,end]
    txd_memcpy(path, url + t, url_len - t);
}

bool parse_http_response(const uint8_t *response, uint32_t response_len, http_response_result_t *result) {
    uint32_t i, p, q, m;
    uint8_t status[4] = {0};
    uint32_t content_length = 0;
    bool ret = false;
    const uint8_t *content_length_buf1 = (uint8_t *)("CONTENT-LENGTH");
    const uint8_t *content_length_buf2 = (uint8_t *)("Content-Length");
    const uint32_t content_length_buf_len = txd_strlen((char *)content_length_buf1);

    // status code
    i = p = q = m = 0;
    for (; i < response_len; ++i) {
        if (' ' == response[i]) {
            ++m;
            if (1 == m) {
                p = i;
            } else if (2 == m) {
                q = i;
                break;
            }
        }
    }
    if (!p || !q || q-p != 4) {
        return false;
    }

    txd_memcpy(status, response+p+1, 3);

    // Content-Length
    p = q = 0;
    for (i = 0; i < response_len; ++i) {
        if (response[i] == '\r' && response[i+1] == '\n') {
            q = i;

            if (!txd_memcmp(response+p, content_length_buf1, content_length_buf_len) ||
                    !txd_memcmp(response+p, content_length_buf2, content_length_buf_len)) {
                int j1 = p+content_length_buf_len, j2 = q-1;
                while ( j1 < q && (*(response+j1) == ':' || *(response+j1) == ' ') ) ++j1;
                while ( j2 > j1 && *(response+j2) == ' ') --j2;
                // [j1,j2]
                uint8_t len_buf[12] = {0};
                txd_memcpy(len_buf, response+j1, j2-j1+1);
                content_length = atoi((char *)len_buf);
                ret = true;
            }
            p = i+2;
        }
        if (response[i] == '\r' && response[i+1] == '\n' &&
                response[i+2] == '\r' && response[i+3] == '\n') {
            p = i+4;
            break;
        }
    }

    if (ret) {
        result->status_code = atoi((char *)status);
        result->body = response + p;
        result->body_len = content_length;
    }

    return ret && (response_len >= p + content_length);  // 期望值是 response_len == p + content_length
}

int32_t txd_http_download(const uint8_t *url, const uint8_t *file_type, const uint8_t *file_key) 
{
    uint32_t timeout_ms = 5000;
    uint32_t request_len = 0;
    http_response_result_t rsp_result = {0};
    txd_socket_handler_t *sock = NULL;
    int32_t ret = 0;
    int32_t result_ret = 0;
    uint8_t *ip = NULL;
    esp_err_t err;
    esp_ota_handle_t update_handle = 0 ;
    struct hostent *h = NULL;
    const esp_partition_t *update_partition = NULL;
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();
    uint8_t *request  = NULL;
    uint8_t *hostname = NULL;
    uint8_t *pathname = NULL;
    uint8_t *response = NULL;

    if (configured != running) {
        WELINK_LOGI("Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",configured->address, running->address);
        WELINK_LOGI("(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    WELINK_LOGI("Running partition type %d subtype %d (offset 0x%08x)",running->type, running->subtype, running->address);
    WELINK_LOGI("Writing to partition subtype %d at offset 0x%x",update_partition->subtype, update_partition->address);

    hostname = (uint8_t *)txd_malloc(255 * sizeof(uint8_t));
    pathname = (uint8_t *)txd_malloc(512 * sizeof(uint8_t));

    if ((hostname == NULL) || (pathname == NULL)) {
        WELINK_LOGE("txd_http_download --- malloc failed\n");
        goto end;
    }

    txd_memset(hostname, 0x0, 255);
    txd_memset(pathname, 0x0, 512);

    get_host_from_url(url, hostname, sizeof(hostname), pathname);
    h = gethostbyname((char *)hostname);
    if (!h) {
        result_ret = -1;
        goto end;
    }
    ip = (uint8_t *)inet_ntoa(*((struct in_addr *)h->h_addr));
    if (!ip) {
        result_ret = -1;
        goto end;
    }

    request  = (uint8_t *)txd_malloc(512 * sizeof(uint8_t));

    if (request == NULL) {
        WELINK_LOGE("txd_http_download --- malloc failed\n");
        goto end;
    }

    txd_memset(request,  0x0, 512);

    snprintf((char *)request, sizeof(request), "GET %s HTTP/1.1\r\nHost:%s\r\nAccept: */*\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n", pathname, hostname);
    request_len = txd_strlen((char *)request);

    if (hostname) {
        txd_free(hostname);
    }

    if (pathname) {
        txd_free(pathname);
    }

    WELINK_LOGI("txd_http_download --- ip[%s]\n", ip);
    WELINK_LOGI("txd_http_download --- request[%s]\n", request);
    // connect server
    sock = txd_tcp_socket_create();
    if (!sock) {
        WELINK_LOGE("txd_http_download --- txd_tcp_socket_create failed\n");
        result_ret = -1;
        goto end;
    }
    ret = txd_tcp_connect(sock, ip, 80, timeout_ms);
    if (ret != 0) {
        WELINK_LOGE("txd_http_download --- txd_tcp_connect failed: err[%d]\n", ret);
        result_ret = -1;
        goto end;
    }

    // send request
    ret = txd_tcp_send(sock, request, request_len, timeout_ms);
    if (ret != request_len) {
        WELINK_LOGE("txd_http_download --- txd_tcp_send failed: request_len[%d] ret[%d]\n", request_len, ret);
        result_ret = -1;
        goto end;
    }

    if (request) {
        txd_free(request);
    }

    response = (uint8_t *)txd_malloc(1024 * sizeof(uint8_t));

    if (response == NULL) {
        WELINK_LOGE("txd_http_download --- malloc failed\n");
        goto end;
    }

    txd_memset(response, 0x0, 1024);

    // receive
    {
        uint32_t idx = 0, temp = 0, lastTime = 0;

        // read header
        // 检查status_code来判断是否已接收完header
        while (0 == rsp_result.status_code) {
            ret = txd_tcp_recv(sock, response+idx, sizeof(response)-idx, timeout_ms);
            if (0 ==ret) {
                WELINK_LOGI("rsp_result.status_code == 0\n");
                continue;
            }
            else if (ret < 0) {
                WELINK_LOGE("txd_http_download --- txd_tcp_recv failed: ret[%d]\n", ret);
                result_ret = -1;
                goto end;
            }
            idx += ret;
            txd_memset(&rsp_result, 0, sizeof(rsp_result));
            parse_http_response(response, idx, &rsp_result);
        }

        if (0 == rsp_result.body_len) {
            WELINK_LOGE("txd_http_download --- rsp_result.body_len[0]\n");
            result_ret = -1;
            goto end;
        }

        err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
        if (err != ESP_OK) {
            while (1) {
                WELINK_LOGE("esp_ota_begin failed, error=%d", err);
                vTaskDelay(3000);
            }
        }
        WELINK_LOGI("esp_ota_begin succeeded");

        // read body
        temp = idx - (rsp_result.body - response);  // response中除header外可能还有部分body数据，temp就是这部分数据的长度
        err = esp_ota_write( update_handle, (const void *)rsp_result.body, temp);
        if (err != ESP_OK) {
            WELINK_LOGE("txd_http_download --- txd_file_write failed: temp[%d] ret[%d]\n", temp, ret);
            result_ret = -1;
            goto end;
        }

        idx = temp;
        while (idx < rsp_result.body_len) {
            ret = txd_tcp_recv(sock, response, sizeof(response), timeout_ms);
            if (0 == ret) {
                continue;
            }
            else if (ret < 0) {
                WELINK_LOGE("txd_http_download --- txd_tcp_recv failed: ret[%d]\n", ret);
                result_ret = -1;
                goto end;
            }
            idx += ret;

            //此处回复下载进度，每3秒发一次
            if (txd_time_get_sysclock() > lastTime + 3000) {
                WELINK_LOGI("txd_http_download -- idx[%d] total[%d]\n", idx, rsp_result.body_len);
                txd_ack_download_progress(0, (uint8_t *)(""), idx, rsp_result.body_len);
                lastTime = txd_time_get_sysclock();
            }

            err = esp_ota_write( update_handle, (const void *)response, ret);
            if (err != ESP_OK) {
                WELINK_LOGE("txd_http_download --- txd_file_write failed: ret[%d]\n", ret);
                result_ret = -1;
                goto end;
            }
        }

        // 回复下载完毕
        txd_ack_download_progress(0, (uint8_t *)(""), idx, rsp_result.body_len);

        WELINK_LOGI("Total Write binary data length : %d", idx);

        if (esp_ota_end(update_handle) != ESP_OK) {
            while (1) {
                WELINK_LOGE("esp_ota_end failed, error=%d", err);
                vTaskDelay(3000);
            }
        }
        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            while (1) {
                WELINK_LOGE("esp_ota_boot partition, error=%d", err);
                vTaskDelay(3000);
            }
        }
        WELINK_LOGI("Prepare to restart system!");
        
        if (response) {
            txd_free(response);
        }

        vTaskDelay(200);
        esp_restart();
    }

end:
    if (sock) {
        txd_tcp_disconnect(sock);
        txd_tcp_socket_destroy(sock);
    }

    if (request) {
        txd_free(request);
    }

    if (hostname) {
        txd_free(hostname);
    }

    if (pathname) {
        txd_free(pathname);
    }

    if (response) {
        txd_free(response);
    }

    WELINK_LOGI("txd_http_download: result_ret[%d] status_code[%d] body_len[%d]\n", result_ret, rsp_result.status_code, rsp_result.body_len);
    return result_ret;
}

void welink_ota_handler(void* arg)
{
    uint8_t msg;

    for (;;) {
        if (pdTRUE == xQueueReceive(welink_ota_queue, &msg, (portTickType)portMAX_DELAY)) {
            WELINK_LOGI("%s:- %s" , __func__,(char *)g_ota_url);
            txd_http_download((uint8_t *)(g_ota_url),NULL,(uint8_t *)("/fatfs/test_file"));
        }
    }
}

bool device_on_new_pkg_come(txd_ota_info_t* pOtaInfo)
{
    uint8_t msg = 0;

    if ((pOtaInfo->target_version == NULL) || (pOtaInfo->md5 == NULL) || (pOtaInfo->url == NULL)) {
        WELINK_LOGE("%s - parameter error!", __func__);
        return false;
    }

    WELINK_LOGI("***device_on_new_pkg_come: pkg_size[%d] target_version[%s] md5[%s] url[%s]\n", pOtaInfo->pkg_size, pOtaInfo->target_version, pOtaInfo->md5, pOtaInfo->url);
    // 不能在这个函数里面下载OTA固件，因为该函数是从SDK回调出来的，所以会阻塞SDK线程，导致不能收发消息

    txd_memcpy(g_ota_url, (char*)pOtaInfo->url, sizeof(g_ota_url));

    if (xQueueSend(welink_ota_queue, &msg, 10 / portTICK_RATE_MS) != pdTRUE) {
        WELINK_LOGE("%s xQueue send failed", __func__);
        return false;
    }
    return true;
}

/****************************消息回调*********************************/
void welink_send_msg_cb(int32_t err_code, uint32_t cookie)
{
    WELINK_LOGI("%s - err_code[%d] cookie[%d]", __func__, err_code, cookie);
}

void welink_receive_datapoint_msg(txd_uint64_t u64SenderId, txd_datapoint_t* datapoint)
{
    txd_datapoint_t datapointAck = {0};
    uint32_t cookie = 0;
    uint8_t *buf = (uint8_t *)txd_malloc(1024 * sizeof(uint8_t));
    uint8_t bufSenderId[30] = {0};

    if (buf != NULL) {
        txd_memcpy(buf, datapoint->property_value, datapoint->property_value_len);
        txd_uint64_to_str(&u64SenderId, bufSenderId);

        WELINK_LOGI("%s - u64SenderId[%s] property_id[%d] property_value_len[%d] property_value[%s] seq[%d]", __func__,
                bufSenderId, datapoint->property_id, datapoint->property_value_len, buf, datapoint->seq);
                
        txd_free(buf);
    }

    datapointAck.property_id = datapoint->property_id;
    datapointAck.seq = datapoint->seq;
    datapointAck.property_value = datapoint->property_value;
    datapointAck.property_value_len = datapoint->property_value_len;
    datapointAck.ret_code = 0;

    txd_report_datapoints(&datapointAck, 1, welink_send_msg_cb, &cookie);
}

/****************************状态回调*********************************/
// 在线状态回调函数，status为1表示在线，为0表示离线（未上线）
void welink_online_status_cb(int32_t status)
{
    if (1 == status) {
        // 测试：
    } else {
        WELINK_LOGE("%s - status[%d]", __func__ , status);
    }
}

// status为1表示已绑定，为0表示未绑定
void welink_bind_status_cb(int32_t status)
{
    WELINK_LOGI("%s - status[%d]", __func__ , status);
}

void welink_err_notify_cb(int32_t err_code)
{
    WELINK_LOGI("%s - err_code[%d]", __func__ , err_code);
}

void esp_welink_handler(void* arg)
{
    uint8_t msg;
    int32_t ret = 0;
    txd_device_info_t info  = {0};
    uint32_t expect_sleep_ms = 0;
    
    info.device_name            = "ESP8266_SDK_TEST";
    info.os_platform            = "RTOS";
    info.product_version        = 0;
    info.product_id             = TEST_PID;
    info.client_pub_key         = CLIENT_PUB_KEY;
    info.client_pub_key_len     = sizeof(CLIENT_PUB_KEY);
    info.auth_key               = AUTH_KEY;
    info.auth_key_len           = sizeof(AUTH_KEY);
    info.device_license         = TEST_SN;
    info.device_serial_number   = TEST_LICENSE;
    info.supportDNS             = 0;

    for (;;) {
        if (pdTRUE == xQueueReceive(welink_task_queue, &msg, (portTickType)portMAX_DELAY)) {

            WELINK_LOGE("%s - %s", info.device_serial_number, info.device_license);

            // 初始化设备信息
            ret = txd_init_device_info(&info);

            if (ret != err_success) {
                WELINK_LOGE("%s - init device info err, errcode[%d]", __func__, ret);
                vTaskDelete(NULL);
            }

            // 初始化接收datapoint消息的回调函数
            txd_init_datapoint(welink_receive_datapoint_msg);

            // 初始化设备通知，包含上线状态，SDK无法处理的错误等
            txd_device_notify_t notify = {0};
            notify.on_online_status = welink_online_status_cb;
            notify.on_err_notify = welink_err_notify_cb;
            txd_init_notify(&notify);

            //OTA
            txd_ota_notify_t ota_notify = {0};
            ota_notify.on_new_pkg_come = device_on_new_pkg_come;
            txd_init_ota(&ota_notify, fw_version);

            welink_ota_queue = xQueueCreate(10, sizeof(uint8_t));
            xTaskCreate(welink_ota_handler, "welink_ota_task", 1024*6, NULL, configMAX_PRIORITIES - 3, NULL);

            // 如果开启了多线程宏(_TXD_THREAD_), 那么内部会启动一个独立线程去执行相关逻辑，SDK线程将伴随整个进程生命周期
            printf("%s - %d - free heap size = %d\r\n", __func__, __LINE__, esp_get_free_heap_size());
            ret = txd_sdk_run(&expect_sleep_ms);

            if (ret != err_success) {
                WELINK_LOGE("%s - sdk run err, errcode[%d]", __func__, ret);
            }

            vTaskDelete(NULL);
        }
    }
}
