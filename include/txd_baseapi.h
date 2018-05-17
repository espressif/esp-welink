/*
 * Copyright (c) 2015 Tencent.
 * All rights reserved.
 */

#ifndef __TXD_BASEAPI_H__
#define __TXD_BASEAPI_H__

#include "txd_stdtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************ baseapi头文件说明 **************************************/
/*
 * sdk依赖的最基本平台功能，此文件内的每个接口接入厂商必须实现，否则链接不过
 */
/*******************************************************************************************/


/************************ memory接口 接入厂商实现 *********************************/
/*
 * 内存申请与释放接口，这样SDK使用者可以做一些内存池相关的策略
 * 目前这里开发者可以直接使用C的malloc与free替代，SDK内部很少动态分配内存
 */

/**  申请内存，同 malloc
 * @param size 内存大小
 *
 * @return 该块内存的首地址
 */
SDK_API extern void* txd_malloc(uint32_t size);


/**  释放内存，同 free
 * @param p txd_malloc返回的指针
 */
SDK_API extern void txd_free(void *p);



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
SDK_API extern int32_t txd_write_basicinfo(uint8_t *buf, uint32_t len);


/** 读取已经持久化的设备基础信息
 * @note 每次调用该函数传入的count的值是固定的，目前为1024
 * @param buf buffer首地址
 * @param len buffer缓冲区大小
 *
 * @return 实际读到的字节数，有错误发生则返回-1
 */
SDK_API extern int32_t txd_read_basicinfo(uint8_t *buf, uint32_t len);



/************************ time接口 接入厂商实现 *********************************/

/**  获得当前系统时钟时间，相对开机时的系统时钟[毫秒数]
 * @note 将超过uint32_t的部分截断即可（SDK内部会维护溢出部分）
 *
 * @return 毫秒数
 */
SDK_API extern uint32_t txd_time_get_sysclock();



/************************** tcp socket接口 接入厂商实现 *****************************/
/*
 * SDK不关心socket的实现是阻塞或非阻塞
 * 注意：SDK内部只会调用一次txd_tcp_socket_create来创建维持TCP长连接（文件传输除外）的socket，
 * 如果连接失败或者断开连接（包括recv和send失败），SDK会先调用txd_tcp_disconnect，然后再调用txd_tcp_connect继续连接
 */

typedef struct txd_socket_handler_t txd_socket_handler_t;


/**  创建tcp socket
 *
 * @return tcp socket
 */
SDK_API extern txd_socket_handler_t* txd_tcp_socket_create();


/**  连接服务器
 * @param sock tcp_socket
 * @param ip 服务器的IP地址，以'\0'结尾的字符串，比如："192.168.1.1"
 * @param port 服务器的端口号，此处为本机字节序，使用时需转成网络字节序
 * @param timeout_ms 超时时间，单位：毫秒
 *
 * @return 0 表示连接成功
 *         -1 表示连接失败
 */
SDK_API extern int32_t txd_tcp_connect(txd_socket_handler_t *sock, uint8_t *ip, uint16_t port, uint32_t timeout_ms);

/**  使用域名连接服务器
 * @param sock tcp_socket
 * @param dns 服务器的域名地址，以'\0'结尾的字符串，比如："devicemsf.3g.qq.com"
 * @param port 服务器的端口号，此处为本机字节序，使用时需转成网络字节序
 * @param timeout_ms 超时时间，单位：毫秒
 *
 * @return 0 表示连接成功
 *         -1 表示连接失败
 */
SDK_API extern int32_t txd_tcp_connect_dns(txd_socket_handler_t *sock, uint8_t *dns, uint16_t port, uint32_t timeout_ms);


/**  断开连接
 * @note 需要保证该socket在断开连接后，还可以调用txd_tcp_connect继续连接服务器
 * @param sock tcp_socket
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_tcp_disconnect(txd_socket_handler_t *sock);


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
SDK_API extern int32_t txd_tcp_recv(txd_socket_handler_t *sock, uint8_t *buf, uint32_t len, uint32_t timeout_ms);


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
SDK_API extern int32_t txd_tcp_send(txd_socket_handler_t *sock, uint8_t *buf, uint32_t len, uint32_t timeout_ms);


/**  销毁tcp socket
 * @param sock tcp_socket
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_tcp_socket_destroy(txd_socket_handler_t *sock);



/************************** sleep接口 接入厂商实现 *****************************/

/**  sleep当前线程，不需要指定线程id，跟linux下的sleep功能（用法）一样，注意这里的单位是ms
 * @note 请注意这里sleep的时间为毫秒
 * @param milliseconds 需要sleep的时间
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_sleep(uint32_t milliseconds);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TXD_BASEAPI_H__ */
