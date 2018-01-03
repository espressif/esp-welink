/*
 * Copyright (c) 2015 Tencent.
 * All rights reserved.
 */

#ifndef __TXD_ERROR_H__
#define __TXD_ERROR_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * global result code
 */
typedef enum {
    err_success                                 = 0x00000000,     //succeed
    err_failed                                  = 0x00000001,     //failed
    err_unknown                                 = 0x00000002,     //未知错误
    err_invalid_param                           = 0x00000003,     //参数非法

    //for check init device param
    err_invalid_device_info                     = 0x0000000C,     //非法的设备信息
    err_invalid_osplatform                      = 0x0000000D,     //platform长度必须小于等于15字节
    err_invalid_devicename                      = 0x0000000E,     //devicename长度必须小于等于32字节
    err_invalid_serial_number                   = 0x0000000F,     //非法的设备序列号
    err_invalid_license                         = 0x00000010,     //非法的license
    err_invalid_clientpubkey                    = 0x00000011,     //非法的clientkey
    err_invalid_ecdhkey                         = 0x00000012,     //非法的ecdhkey
    err_hardware_null                           = 0x00000013,     //hardwareInfo未初始化

    //check
    err_not_connect_succ                        = 0x00000031,     //连接Server失败
    err_not_register_succ                       = 0x00000032,     //没有注册成功
    err_not_login_succ                          = 0x00000033,     //没有登陆成功
    err_not_online_succ                         = 0x00000034,     //没有上线成功

    //for msg
    err_msg_sendtimeout                         = 0x00020002,     //消息发送超时
    err_msg_buff_too_large                      = 0x00020003,     //消息buffer超过最大限制
    err_msg_encode_failed                       = 0x00020004,     //消息编码失败
    err_msg_encode_json_failed                  = 0x00020005,     //结构化消息编码失败
    err_msg_cache_failed                        = 0x00020006,     //消息缓存失败，一般是超过缓存限制
    err_msg_send_too_frequently                 = 0x00020007,     //消息发送太频繁
} txd_result_t;


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TXD_ERROR_H__ */
