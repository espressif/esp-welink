/*
 * Copyright (c) 2015 Tencent.
 * All rights reserved.
 */

#ifndef __TXD_OTA_H__
#define __TXD_OTA_H__

#include "txd_stdtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     *  OTA升级方案：
     *
     *
     *    step1. 升级前，首先需要接入厂商在微瓴的后台管理配置页面配置新版本固件信息，后台根据配置信息以及设备初始化时填入的ota_version来确定是否配置升级任务
     *    step2. 后台下发ota升级任务后，SDK通过on_new_pkg_come回调通知设备，设备可以通过txd_ota_info_t中的信息下载和验证固件包
     *    step3. 设备将下载进度通过txd_ack_download_progress接口通知SDK，SDK会将进度上报。
     *    step4. 升级完成后设备调用txd_ack_ota_result接口通知SDK，并更新固件版本号
     */
typedef struct {
    uint8_t      md5[32];         //升级包md5，用于文件检验
    uint32_t     md5_len;
    uint8_t      url[512];        //升级包的下载地址，需要按照标准的http协议去下载
    uint32_t     url_len;
    uint32_t     pkg_size;        //新的升级固件包的大小
    uint8_t      desc[100];       //title + desc: 升级描述信息，如果您的智能设备没有显示屏，可以忽略
    uint32_t     desc_len;
    uint8_t      target_version[100];  //升级目标版本
    uint32_t     target_version_len;
    
} txd_ota_info_t;

typedef struct {
    /**  step2. 收到后台下发ota升级任务
     * @remarks 有设备可用的新固件版本，手机会将查询到的固件包信息通知给设备
     * @param u64SenderId 消息发送者（手Q）的ID
     * @param pOtaInfo 升级包的相关信息
     *
     * @return true sdk将会开始启动升级包下载
     *         false 会提示用户设备端拒绝升级（一般是磁盘(flash)剩余空间问题）
     */
    bool (*on_new_pkg_come)(txd_ota_info_t *pOtaInfo);

} txd_ota_notify_t;


/**  step1. 初始化OTA 设置回调和当前版本信息
 * @param notify OTA的回调通知
 * @param current_version 当前版本号
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API int32_t txd_init_ota(txd_ota_notify_t *notify, uint8_t *current_version);


/**  step3. 设备下载升级文件，并且实时地将进度通知给SDK
 * @note 注意该函数不能在SDK的回调函数里面调用，否则会把消息发送队列填满，导致发不出消息
 *               另外建议调用该函数的频率为3--5秒一次（否则容易阻塞发送队列）
 * @param ret_code 0表示成功，其他表示失败
 * @param err_msg 失败原因描述
 * @param download_size 当前已经下载到的文件大小
 * @param total_size 文件总大小
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_ack_download_progress(int32_t ret_code, uint8_t *err_msg, uint32_t download_size, uint32_t total_size);


/**  step4. 设备升级完成后通知SDK，并更新固件版本号
 * @param ret_code 0表示成功；1表示失败
 * @param err_msg 升级失败的描述文案，升级失败时填写
 * @param current_version 当前版本号
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_ack_ota_result(int32_t ret_code, uint8_t *err_msg, uint8_t *current_version);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TXD_OTA_H__ */
