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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <sys/time.h>

#include "netdb.h"
#include "txd_stdtypes.h"
#include "txd_baseapi.h"
#include "esp32_welink_log.h"

static const char* TAG = "txd_baseapi";
#define ESP_FATFS_BASE_PATH "/fatfs"

/************************ memory接口 接入厂商实现*********************************/
/*
 * 内存申请与释放接口，这样SDK使用者可以做一些内存池相关的策略
 * 目前这里开发者可以直接使用C的malloc与free替代，此接口很少使用后续考虑删除
 */
/*
 * 申请内存，同malloc
 */
void* txd_malloc(uint32_t size)
{
    return malloc(size);
}

/*
 * 释放内存，同free
 */
void txd_free(void* p)
{
    free(p);
}

/************************** store接口 接入厂商实现 ******************************/
/*
 * 持久化数据时，SDK不关心具体写到哪个位置，但需要保证每次调用txd_write_basicinfo将覆盖掉以前的数据，
 * 并且调用txd_read_basicinfo能够读取到这些数据。
 * 目前需要开辟1K的flash存储空间
 */

/**  将设备的基础信息持久化
 * @note 将buf指针所指的内存写入len个字节到文件（flash）中，每次调用该函数传入的len的值可能不一样
 * @param buf buffer首地址
 * @param len buffer长度
 *
 * @return 实际写入的字节数，有错误发生则返回-1
 */
int32_t txd_write_basicinfo(uint8_t* buf, uint32_t count)
{
    int32_t ret = -1;
    char* filename = (char*)txd_malloc(strlen(ESP_FATFS_BASE_PATH) + strlen("esp32_tencent_basicinfo") + 2);

    if (filename == NULL) {
        WELINK_LOGE("malloc fail");
        return ret;
    }

    sprintf((char*)filename, "%s/%s", ESP_FATFS_BASE_PATH, "esp32_tencent_basicinfo");
    FILE* f = fopen(filename, "wb");

    if (f == NULL) {
        WELINK_LOGE("fopen fail");
        txd_free(filename);
        return ret;
    }

    ret = fwrite(buf, 1, count, f);
    fclose(f);
    txd_free(filename);
    return ret;
}

/** 读取已经持久化的设备基础信息
 * @note 每次调用该函数传入的count的值是固定的，目前为1024
 * @param buf buffer首地址
 * @param len buffer缓冲区大小
 *
 * @return 实际读到的字节数，有错误发生则返回-1
 */
int32_t txd_read_basicinfo(uint8_t* buf, uint32_t count)
{
    int32_t ret = -1;
    char* filename = (char*)txd_malloc(strlen(ESP_FATFS_BASE_PATH) + strlen("esp32_tencent_basicinfo") + 2);

    if (filename == NULL) {
        WELINK_LOGE("malloc fail");
        return ret;
    }

    sprintf((char*)filename, "%s/%s", ESP_FATFS_BASE_PATH, "esp32_tencent_basicinfo");

    FILE* f = fopen(filename, "rb");

    if (f == NULL) {
        WELINK_LOGE("fopen fail");
        txd_free(filename);
        return ret;
    }

    ret = fread(buf, 1, count, f);
    txd_free(filename);
    fclose(f);
    return ret;
}

/************************ time接口 接入厂商实现 *********************************/

/**  获得当前系统时钟时间，相对开机时的系统时钟[毫秒数]
 * @note 将超过uint32_t的部分截断即可（SDK内部会维护溢出部分）
 *
 * @return 毫秒数
 */
uint32_t txd_time_get_sysclock()
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return esp_log_early_timestamp();
    }

    static uint32_t base = 0;

    if (base == 0 && xPortGetCoreID() == 0) {
        base = esp_log_early_timestamp();
    }

    return base + xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
}

/************************** tcp socket接口 接入厂商实现 *****************************/
/*
 * SDK不关心socket的实现是阻塞或非阻塞
 * 注意：SDK内部只会调用一次txd_tcp_socket_create来创建维持TCP长连接（文件传输除外）的socket，
 * 如果连接失败或者断开连接（包括recv和send失败），SDK会先调用txd_tcp_disconnect，然后再调用txd_tcp_connect继续连接
 */

struct txd_socket_handler_t {
    int fd;
};

/**  创建tcp socket
 *
 * @return tcp socket
 */
txd_socket_handler_t* txd_tcp_socket_create()
{
    return (txd_socket_handler_t*)txd_malloc(sizeof(txd_socket_handler_t));
}

/**  连接服务器
 * @param sock tcp_socket
 * @param ip 服务器的IP地址，以'\0'结尾的字符串，比如："192.168.1.1"
 * @param port 服务器的端口号，此处为本机字节序，使用时需转成网络字节序
 * @param timeout_ms 超时时间，单位：毫秒
 *
 * @return 0 表示连接成功
 *         -1 表示连接失败
 */
int32_t txd_tcp_connect(txd_socket_handler_t* sock, uint8_t* ip, uint16_t port, uint32_t timeout_ms)
{
    int opt = 1;
    int32_t ret = -1;
    struct sockaddr_in addr;
    struct timeval timeout = {0, 0};

    if ((sock == NULL) || (ip == NULL)) {
        WELINK_LOGE("the parameter is incorrect");
        return ret;
    }

    sock->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock->fd < 0) {
        WELINK_LOGE("create socket fail");
        return ret;
    }

    if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        WELINK_LOGE("set socket opt fail");
        close(sock->fd);
        return ret;
    }

    timeout.tv_sec = (timeout_ms / 1000);
    timeout.tv_usec = ((timeout_ms % 1000) * 1000);

    if (setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(struct timeval)) != 0) {
        WELINK_LOGE("set socket opt fail");
        close(sock->fd);
        return ret;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton((char*)ip, &(addr.sin_addr.s_addr));

    if (connect(sock->fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        WELINK_LOGE("socket connect fail");
        return ret;
    }

    WELINK_LOGI("socket connect success");
    return 0;
}

/**  使用域名连接服务器
 * @param sock tcp_socket
 * @param dns 服务器的域名地址，以'\0'结尾的字符串，比如："devicemsf.3g.qq.com"
 * @param port 服务器的端口号，此处为本机字节序，使用时需转成网络字节序
 * @param timeout_ms 超时时间，单位：毫秒
 *
 * @return 0 表示连接成功
 *         -1 表示连接失败
 */
int32_t txd_tcp_connect_dns(txd_socket_handler_t* sock, uint8_t* dns, uint16_t port, uint32_t timeout_ms)
{
    int opt = 1;
    int32_t ret = -1;
    struct sockaddr_in addr;
    struct hostent* hptr = NULL;
    ip_addr_t ip_address;
    struct timeval timeout = {0, 0};

    if ((sock == NULL) || (dns == NULL)) {
        WELINK_LOGE("the parameter is incorrect");
        return ret;
    }

    sock->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock->fd < 0) {
        WELINK_LOGE("create socket fail");
        return ret;
    }

    if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        WELINK_LOGE("set socket opt fail");
        close(sock->fd);
        return ret;
    }

    timeout.tv_sec = (timeout_ms / 1000);
    timeout.tv_usec = ((timeout_ms % 1000) * 1000);

    if (setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(struct timeval)) != 0) {
        WELINK_LOGE("set socket opt fail");
        close(sock->fd);
        return ret;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if ((hptr = gethostbyname((char*)dns)) == NULL) {
        WELINK_LOGE("gethostbyname fail");
        return ret;
    }

    ip_address = *(ip_addr_t*)hptr->h_addr_list[0];
    addr.sin_addr.s_addr = ip_address.u_addr.ip4.addr;

    if (connect(sock->fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        WELINK_LOGE("socket connect fail - DNS");
        return ret;
    }

    WELINK_LOGI("socket connect success - DNS");

    return 0;
}

/**  断开连接
 * @note 需要保证该socket在断开连接后，还可以调用txd_tcp_connect继续连接服务器
 * @param sock tcp_socket
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
int32_t txd_tcp_disconnect(txd_socket_handler_t* sock)
{
    int32_t ret = -1;

    if (sock == NULL) {
        WELINK_LOGE("the parameter is incorrect");
        return ret;
    }

    ret = close(sock->fd);
    sock->fd = -1;
    return ret;
}

/**  接收数据
 * @remarks 从socket中接收数据到buf[0:len)中，超时时间为timeout_ms毫秒
 * @param sock tcp_socket
 * @param buf 接收缓冲区的首地址
 * @param len 接收缓冲区buf的大小
 * @param timeout_ms 超时时间，单位：毫秒
 *
 * @return -1 表示发生错误，SDK在发现返回-1后会调用txd_tcp_disconnect
 *         0 表示在timeout_ms时间内没有收到数据，属于正常情况
 *         正数 表示收到的字节数
 */
int32_t txd_tcp_recv(txd_socket_handler_t* sock, uint8_t* buf, uint32_t len, uint32_t timeout_ms)
{
    int32_t ret = -1;
    int32_t optval = 0;
    socklen_t optlen = 0;
    struct timeval timeout = {0, 0};

    if ((sock == NULL) || (buf == NULL)) {
        WELINK_LOGE("the parameter is incorrect");
        return ret;
    }

    timeout.tv_sec = (timeout_ms / 1000);
    timeout.tv_usec = ((timeout_ms % 1000) * 1000);

    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval)) != 0) {
        WELINK_LOGE("set socket opt fail");
        return ret;
    }

    ret = recv(sock->fd, buf, len, 0);

    if (ret == -1) {
        optlen = sizeof(optval);

        if (getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) == 0) {
            if (optval == EAGAIN) {
                return 0;
            }
        }
    }

    return ret;
}

/**  发送数据
 * @param sock tcp_socket
 * @param buf 待发送数据的首地址
 * @param len 待发送数据的大小（字节数）
 * @param timeout_ms 超时时间，单位：毫秒
 *
 * @return -1 表示发生错误，SDK在发现返回-1后会调用txd_tcp_disconnect
 *         0 表示再timeout_ms时间内没有将数据发送出去
 *         正数 表示发送的字节数
 */
int32_t txd_tcp_send(txd_socket_handler_t* sock, uint8_t* buf, uint32_t len, uint32_t timeout_ms)
{
    int32_t ret = -1;
    struct timeval timeout = {0, 0};

    if ((sock == NULL) || (buf == NULL)) {
        WELINK_LOGE("the parameter is incorrect");
        return ret;
    }

    timeout.tv_sec = (timeout_ms / 1000);
    timeout.tv_usec = ((timeout_ms % 1000) * 1000);

    if (setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(struct timeval)) != 0) {
        WELINK_LOGE("set socket opt fail");
        return ret;
    }

    return send(sock->fd, buf, len, 0);
}

/**  销毁tcp socket
 * @param sock tcp_socket
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
int32_t txd_tcp_socket_destroy(txd_socket_handler_t* sock)
{
    int32_t ret = -1;

    if (sock) {
        ret = close(sock->fd);
        txd_free(sock);
    }

    return ret;
}

/************************** sleep接口 接入厂商实现*****************************/
/*
 * sleep当前线程，不需要指定线程id，跟linux下的sleep功能（用法）一样，注意这里的单位是ms
 */
int32_t txd_sleep(uint32_t milliseconds)
{
    vTaskDelay(milliseconds / (1000 / xPortGetTickRateHz()));
    return 0;
}

