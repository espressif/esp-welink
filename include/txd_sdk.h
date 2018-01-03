/*
 * Copyright (c) 2015 Tencent.
 * All rights reserved.
 */

#ifndef __TXD_SDK_H__
#define __TXD_SDK_H__

#include "txd_stdtypes.h"
#include "txd_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************ RTOS SDK 基本介绍 **************************************/
/*
 * 为了实现SDK系统无关性，抽象出GKI层接口，开发者首先需要实现这些接口：如store、time、socket等， 具体参见头文件：
 * 基础接口必须实现：  txd_stdapi.h  txd_baseapi.h
 * 可选模块实现接口：  txd_file.h    txd_thread.h   txd_qq_link.h


 * 为了适应不同产品的需求，SDK各个模块可拆卸，开发者需要明确使用哪些功能，可拆卸模块如下；
-------------------|-----------------------------------------------|----------------------------------------|
           模块     |                    简介                        |         实现接口                        |
-------------------|-----------------------------------------------|----------------------------------------|
    qqlink模块      |  非博通芯片支持qqlink方式给设备设置wifi密码；        |  txd_qq_link.h                         |
    文件传输模块      |  系统必须支持线程才可使用；必须实现                  |  txd_file.h  txd_thread.h              |
    线程模块         |  区分系统是否需要使用线程；                        |  txd_thread.h                          |
-------------------|-----------------------------------------------|----------------------------------------|
 * qqlink、文件传输、线程可选；


 * SDK使用步骤：
 *          1.实现GKI层接口并初始化GKI接口；
 *          2.SDK设备信息初始化并运行；
 *          3.使用DataPoint接口实现消息收发；
 *          4.event notify上线后才能收发消息，开发者也可不关心；
 *
 */


/********************************** SDK 设备信息初始化并运行 *********************************/

/*
 * 操作系统信息：设备厂商自行填入
 * 网络信息：设备厂商自行填入
 * 硬件信息、即Server分配的信息、公私钥如何获取请参见QQ物联接入流程，需申请产品
 */
typedef struct {
    //操作系统信息
    char*                 os_platform;          //操作系统类型:threadx，freertos，etc, 以'\0'结尾的字符串，长度不超过15字节

    //硬件信息
    char*                 device_name;          //设备名称,'\0'结尾字符串长度不超过32字节，必须与网站注册的devicename一致
    char*                 device_serial_number; //设备序列号,'\0'结尾字符串长度必须是16字节
    char*                 device_license;       //设备LICENSE，'\0'结尾字符串
    int32_t               product_id;           //每一个厂商的每一种类型的设备对应的一个id
    int32_t               product_version;      //产品的版本信息，用于OTA，默认填0

    //密钥信息
    uint8_t*              client_pub_key;       //client公钥，每台设备生成后烧录到设备
    int32_t               client_pub_key_len;
    uint8_t*              auth_key;             //authkey，每台设备生成后烧录到设备
    int32_t               auth_key_len;

    uint8_t               supportDNS;           //是否支持域名解析
} txd_device_info_t;


/**  初始化设备信息
 * @param device_info 设备信息
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_init_device_info(const txd_device_info_t* device_info);


/**  SDK启动函数
 * @remarks 支持线程的系统，txd_sdk_run只需要调用一次，SDK会启动一个线程独立运行，不影响开发者主线程；
 *          不支持线程的系统，需要在开发者主线程里面循环调用txd_sdk_run
 *          若开发者是单线程模式(SDK没有开启_TXD_THREAD_宏)，那么需要关注expect_sleep_ms出参，
 *          这是SDK期望由开发者在上一层进行sleep（单位毫秒），可以根据自己的实际情况进行处理。
 *          （若没有进行sleep，SDK可能会频繁的请求server，跑满cpu，导致耗电量增加）
 * @note 使用QQLink的需要在连上WIIF后才能调用txd_sdk_run
 * @param expect_sleep_ms 出参，SDK期望sleep的时间，单位：毫秒
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_sdk_run(uint32_t* expect_sleep_ms);


/**   获取SDK版本号
 * @param main_version 主版本号
 * @param sub_version 子版本号
 * @param build_number 编译号
 */
SDK_API void txd_get_sdk_version(uint32_t* main_version, uint32_t* sub_version, uint32_t* build_number);



/************************************ event notification ***********************************/

typedef enum {
    err_notify_udp_port_used            = 0x00000001,     //udp端口被占用
    err_notify_tcp_connect_failed       = 0x00000002,     //tcp若干次连接失败会通知这个错误给厂家，厂家可以采取相关措施解决，SDK内部会不断尝试连接
} txd_err_t;

/*
 * 设备相关的事件通知
 */
typedef struct {
    /**  上下线通知
     * @remarks 每次上下线切换的时候会回调这个通知
     * @param status 1表示在线，0表示离线（未上线）
     */
    void (*on_online_status)(int32_t status);


    /**  绑定/解绑通知
     * @remarks SDK在发起登录后，会通知外面该设备是否已经被绑定，而后，只有当设备被绑定或解绑的时候才会回调通知
     * @param status 1表示已绑定，0表示未绑定
     */
    void (*on_bind_status)(int32_t status);


    /**  错误通知
     * @remarks SDK遇到无法处理的错误通知出来
     * @param code 错误码参见txd_err_t
     */
    void (*on_err_notify)(int32_t code);

} txd_device_notify_t;


/**  初始化设备相关事件通知
 * @note 如果不关注txd_device_notify_t里面的某些事件，可以把对应的函数指针填为空
 * @param notify 设备相关的事件通知
 */
SDK_API void txd_init_notify(const txd_device_notify_t* notify);



/******************** datapoint消息，注意用于设备与手机QQ轻App(H5页面)页面通信 ***************************/

typedef struct  {
    uint32_t property_id;         //属性ID
    uint8_t* property_value;      //属性value，类型为字节数组
    uint32_t property_value_len;  //属性value的长度
    uint32_t seq;                 //操作序号
    int32_t  ret_code;            //收到cc datapoint，回复ACK时设置此值供H5页面使用
} txd_datapoint_t;


/**  消息发送是否成功的回调
 * @param err_code 0表示成功，其余见错误码表
 * @param cookie cookie是在调用相应send函数时从出参中获取的，用于标记消息
 */
typedef void (*on_send_msg)(int32_t err_code, uint32_t cookie);


/**  接收datapoint消息的回调函数
 * @param u64SenderId 发送者ID（app端）
 * @param datapoint 单个datapoint结构体
 */
typedef void (*on_receive_cc_datapoint)(txd_uint64_t u64SenderId, txd_datapoint_t* datapoint);


/** 设置接收datapoint消息的回调
 * @param pCb 回调函数
 */
SDK_API void txd_init_datapoint(const on_receive_cc_datapoint pCb);


/**  向指定u64ReceiverId发送datapoint消息，用于对H5页面发送消息的回复
 * @remarks property_id和seq应该与收到的datapoint里面的值一样，property_value、property_value_len、ret_code这三个值可以自己填写
 * @note datapoint中的property_value_len不应该超过480
 *       每秒最多调用该函数5次，否则返回错误码err_msg_send_too_frequently
 *       property_value为字符串类型
 * @param u64ReceiverId 接收者ID（app端）
 * @param datapoint 单个datapoint结构体
 * @param pCb 发送是否成功的回调函数
 * @param pCookie 用于标记该消息，会在回调函数（pCb）中返回
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_ack_cc_datapoint(txd_uint64_t u64ReceiverId, txd_datapoint_t* datapoint, on_send_msg pCb, uint32_t* pCookie);


/**  设备状态上报到后台服务器，手Q测H5通过query接口查询上报的状态
 * @remarks datapoint结构里面只需要填写property_id、property_value、property_value_len这3个字段（seq和ret_code可以统一填0）
 * @note datapoints数组所有的property_value_len相加不应该超过480
 *       每秒最多调用该函数5次，否则返回错误码err_msg_send_too_frequently
 *       property_value为字符串类型
 * @relates 这里为datapoint数组的原因是可以将多个datapoint消息打包到一条消息里面发送，
 *          尽量避免占用发送缓冲区（如果超过内部的发送缓冲区大小，则会返回错误）
 * @param datapoints datapoint数组
 * @param datapoints_count datapoint的个数
 * @param pCb 发送是否成功的回调函数
 * @param pCookie 用于标记该消息，会在回调函数（pCb）中返回
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_report_datapoints(txd_datapoint_t datapoints[], uint32_t datapoints_count, on_send_msg pCb, uint32_t* pCookie);



/************************************ 与手机QQ互发消息 **************************************/

/**  接收文本消息的回调
 * @param u64SenderId 发送者ID（app端）
 * @param timestamp 消息发送的时间戳（秒）
 * @param text 文本消息，以'\0'结尾的字符串
 * @param text_len 文本消息长度
 */
typedef void (*on_receive_text_msg)(txd_uint64_t u64SenderId, uint32_t timestamp, uint8_t* text, uint32_t text_len);


/** 设置接收文本消息的回调
 * @param pCb 回调函数
 */
SDK_API void txd_init_text_msg(const on_receive_text_msg pCb);


/**  发送文本消息
 * @remarks 该文本消息将在手Q的聊天窗口显示
 * @note 每秒最多调用该函数5次，否则返回错误码err_msg_send_too_frequently
 * @param u64ReceiverId 接收者ID（app端），如果u64ReceiverId为0（8个字节全部为0），则表示该条消息将发往所有的设备绑定者（主人和被分享者）
 * @param pMsgText 以'\0'结尾的字符串，长度最长为400
 * @param pCb 发送是否成功的回调函数
 * @param pCookie 用于标记该消息，会在回调函数（pCb）中返回
 * @param value 该值匹配命中open.qq.com平台里所配置的触发器后，会触发相应的动作
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_send_text_msg(txd_uint64_t u64ReceiverId, uint8_t* pMsgText, on_send_msg pCb, uint32_t* pCookie, int32_t value);


/** 发送强提醒通知
 * @remarks 手机锁屏时也能看到，类似来电通知界面
 * @note 每秒最多调用该函数5次，否则返回错误码err_msg_send_too_frequently
 * @param u64ReceiverId 接收者ID（app端），如果u64ReceiverId为0（8个字节全部为0），则表示该条消息将发往所有的设备绑定者（主人和被分享者）
 * @param pDigestText 以'\0'结尾的字符串，长度最长为400
 * @param pCb 发送是否成功的回调函数
 * @param pCookie 用于标记该消息，会在回调函数（pCb）中返回
 * @param value 该值匹配命中open.qq.com平台里所配置的触发器后，会触发相应的动作
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_send_notify_msg(txd_uint64_t u64ReceiverId, uint8_t* pDigestText, on_send_msg pCb, uint32_t* pCookie, int32_t value);


/***************** 解绑设备，所有的绑定关系将解除，设备将处于未绑定状态 ********************/

/** 解绑的回调
 * @param err_code 0表示解绑成功，否则表示解绑失败
 */
typedef void (*on_erase_binders)(int32_t err_code);


/** deprecated, 设备主动解除所有绑定者，此接口不推荐使用
 * @param pCb 接绑结果由 pCb回调出来
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_erase_binders(on_erase_binders pCb);


/********************************** 获取设备的状态：是否在线，是否被绑定******************************/

/**  获取设备的在线状态
 *
 * @return 1 表示在线
 *         0 表示离线
 */
SDK_API int32_t txd_get_online_status();


/**  获取设备的绑定状态
 *
 * @return 1 表示已经被绑定
 *         0 表示未被绑定
 */
SDK_API int32_t txd_get_bind_status();


/**  获取服务器标准校时时间
 * @note 如果没有登录成功，则此接口只返回0
 *
 *  @return 服务器校时时间（Unix时间戳），SDK内部使用系统时钟（txd_time_get_sysclock函数）做累加
 */
SDK_API uint32_t txd_get_server_time();


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TXD_SDK_H__ */
