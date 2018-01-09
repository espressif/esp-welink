/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
#include "txd_baseapi.h"

/************************** tcp socket接口 接入厂商实现 *****************************/
/*
 * SDK不关心socket的实现是阻塞或非阻塞
 * 注意：SDK内部只会调用一次txd_tcp_socket_create来创建维持TCP长连接（文件传输除外）的socket，
 * 如果连接失败或者断开连接（包括recv和send失败），SDK会先调用txd_tcp_disconnect，然后再调用txd_tcp_connect继续连接
 */

struct txd_socket_handler_t {
    int fd;
};

/*
 * 创建tcp socket
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
        return ret;
    }

    sock->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock->fd < 0) {
        return ret;
    }

    if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        return ret;
    }

    timeout.tv_sec = (timeout_ms / 1000);
    timeout.tv_usec = ((timeout_ms % 1000) * 1000);

    if (setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(struct timeval)) != 0) {
        return ret;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton((char*)ip, &(addr.sin_addr.s_addr));

    if (connect(sock->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        return ret;
    }

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
    struct timeval timeout = {0, 0};

    if ((sock == NULL) || (buf == NULL)) {
        return ret;
    }

    timeout.tv_sec = (timeout_ms / 1000);
    timeout.tv_usec = ((timeout_ms % 1000) * 1000);

    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval)) != 0) {
        return ret;
    }

    return recv(sock->fd, buf, len, 0);
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
        return ret;
    }

    timeout.tv_sec = (timeout_ms / 1000);
    timeout.tv_usec = ((timeout_ms % 1000) * 1000);

    if (setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(struct timeval)) != 0) {
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
    if (sock) {
        txd_free(sock);
    }

    return 0;
}
