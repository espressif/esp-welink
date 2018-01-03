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

/*
 *  QQ物联升级方案：
 *
 *                                                       |—————————————|
 *                                                       |   server    |
 *                                                       |—————————————|
 *                                                             |
 *                                                         1.query
 *                                                             |
 *      |-----------|                                    |—————————————|
 *      |           |    <---- 2.on_new_pkg_come ------  |             |
 *      |  DEVICE   |    ----- 3.txd_ack_download_progress --->  |     APP     |
 *      |           |    <---- 4.on_update_confirm ----  |             |
 *      |           |                                    |             |
 *      |-----------|    ----- 5.txd_ack_ota_result ------>  |—————————————|
 *
 *
 *    step1. 手机向QQ物联升级服务器查询是否有新的升级信息，升级信息需要您在QQ物联的官网（iot.qq.com）的后台管理配置页面进行配置
 *           后台根据配置信息以及设备初始化时填入的product_version大小来确定是否有版本升级，建议在测试时单独准备两个版本号，不要和正式版本号混用，
 *           以免对正式设备造成影响。
 *    step2. 有设备可用的新固件版本，手机会将查询到的固件包信息通知给设备
 *    step3. 设备下载升级文件，并且实时地将进度通知给手机QQ
 *    step4. 手机等设备将文件下载完成后，会有一个最后的用户确认才会开始更新固件，因为替换文件可能需要一些时间，
 *           之后也有可能要重启设备，一个手机端的确认界面能给用户以心理上的等待预期。
 *    step5. 设备完成文件替换（并且重启之后），需要给手机一个升级结果的反馈，告知手机是否升级成功了
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
    /**  step2. 收到APP的升级请求
     * @remarks 有设备可用的新固件版本，手机会将查询到的固件包信息通知给设备
     * @param u64SenderId 消息发送者（手Q）的ID
     * @param pOtaInfo 升级包的相关信息
     *
     * @return true sdk将会开始启动升级包下载
     *         false 会提示用户设备端拒绝升级（一般是磁盘(flash)剩余空间问题）
     */
    bool (*on_new_pkg_come)(txd_ota_info_t* pOtaInfo);

} txd_ota_notify_t;


/**  初始化OTA
 * @param notify OTA的回调通知
 * @param replace_timeout step.4 -> step.5 希望手机APP最多等待多少时间提示升级超时，需要保证绝大多数情况下：
 *                                         文件替换 + 设备重启的时间 < replace_timeout, 时间单位:秒
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API int32_t txd_init_ota(txd_ota_notify_t* notify, uint8_t* current_version);


/**  step3. 发送下载进度
 * @note 注意该函数不能在SDK的回调函数里面调用，否则会把消息发送队列填满，导致发不出消息
 *               另外建议调用该函数的频率为3--5秒一次（否则容易阻塞发送队列）
 * @remarks 设备下载升级文件，并且实时地将进度通知给手机QQ
 * @param ret_code 0表示成功，其他表示失败
 * @param download_size 当前已经下载到的文件大小
 * @param total_size 文件总大小
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_ack_download_progress(int32_t ret_code, uint8_t* err_msg, uint32_t download_size, uint32_t total_size);


/**  step5. 发送升级结果
 * @remarks 设备完成文件替换（并且重启之后），需要给手机一个升级结果的反馈，告知手机是否升级成功了,否则手q会在一段时间
 *          之后告知用户升级超时，所以请务必实现此接口（如果设备重启，需要在设备上线之后调用）
 * @param ret_code 0表示成功；1表示失败
 * @param err_msg 升级失败的描述文案，升级失败时填写
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_ack_ota_result(int32_t ret_code, uint8_t* err_msg, uint8_t* current_version);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TXD_OTA_H__ */
