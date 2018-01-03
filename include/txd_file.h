/*
 * Copyright (c) 2015 Tencent.
 * All rights reserved.
 */

#ifndef __TXD_FILE_H__
#define __TXD_FILE_H__

#include "txd_stdtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 文件操作接口
 */
typedef struct txd_file_handler_t txd_file_handler_t;

/*
 * 文件打开模式
 */
typedef enum {
    fmode_r   = 1,   //只读
    fmode_w   = 2,   //只写。如果文件不存在，则创建新文件，否则将清空文件，再写入。
} txd_file_mode_t;

/*
 * 文件具有的基本属性
 */
typedef struct {
    uint8_t  file_type[10];   //文件类型，一定要填后缀名，不能带小数点，比如："jpg", "arm", "mp3", "mp4", "avi" 等
    uint32_t file_type_len;   //file_type的长度
    uint8_t  file_key[60];    //用于标记该文件（具有唯一性），可以看作是文件名，可以带后缀，也可以不带
    uint32_t file_key_len;    //filekey的长度，最多不超过60字节
    uint32_t file_size;       //文件大小，单位：字节
} txd_file_info_t;


/********************************* file接口 *************************************/

/**  以mode方式打开文件，文件类型为file_type
 * @note 开发者在上传文件时，需要传入file_type和file_key，在收到文件并下载时，file_type由服务器下发，file_key为空（应该由该函数内部生成，SDK会调用txd_file_get_info来获取file_key）
 * @param file_type 文件类型，'\0'结尾字符串，即文件后缀，如"avi"，该值可能为空
 * @param file_key 唯一标记该文件，以'\0'结尾的字符串，如果file_key为空，函数内部应该自己生成file_key
 * @param mode 打开方式
 *
 * @return 文件（句柄）标记
 */
SDK_API extern txd_file_handler_t* txd_file_open(uint8_t* file_type, uint8_t* file_key, txd_file_mode_t mode);


/**  从file中读取数据到buf[0:n)中
 * @param file 文件（句柄）标记
 * @param buf 接收缓冲区的首地址
 * @param len 接收缓冲区buf的大小
 *
 * @return -1 表示发生错误
 *         0 表示已无数据可读（已经读取完毕）
 *         正数 表示读到的字节数
 */
SDK_API extern int32_t txd_file_read(txd_file_handler_t* file, uint8_t* buf, uint32_t len);


/**  向file中写入buf[0:n)
 * @param file 文件（句柄）标记
 * @param buf 待写入数据的缓冲区首地址
 * @param len 待写入数据的大小（字节数）
 *
 * @return -1 表示发生错误
 *         0 表示没有写入数据
 *         正数 表示写入的字节数
 */
SDK_API extern int32_t txd_file_write(txd_file_handler_t* file, uint8_t* buf, uint32_t len);


/**  获取file的基本属性
 * @param file 文件（句柄）标记
 * @param info 文件信息，出参
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_file_get_info(txd_file_handler_t* file, txd_file_info_t* info);


/**  移动文件读写指针到文件的第pos个字节
 * @param file 文件（句柄）标记
 * @param pos 文件的相对位置（base-zero）
 *            pos为0：表示指向文件头部，也就是刚打开文件的状态
 *            pos为0xffffffff：表示指向文件末尾，无需知道文件的大小
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_file_seek(txd_file_handler_t* file, uint32_t pos);


/**  关闭并保存文件
 * @param file 文件（句柄）标记
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_file_close(txd_file_handler_t* file);


/**  删除文件
 * @note SDK在下载完文件后会对文件做校验，如果校验失败，则会调用txd_file_remove将其删除
 *       如果该文件已经被打开，SDK会先调用txd_file_close将其关闭，然后再调用txd_file_remove
 * @param file_key 唯一标记该文件，以'\0'结尾的字符串
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_file_remove(uint8_t* file_key);



/*********************************** 语音文件消息收发 **************************************/

/**  接收语音消息的回调函数
 * @param audio_file_key 文件的唯一标记
 * @param audio_file_type 语音文件类型，暂只支持"amr"
 * @param audio_duration 语音时长，单位：秒
 * @param msg_time 消息发送的时间戳，单位：秒
 */
typedef void (*on_receive_audio_msg)(uint8_t* audio_file_key, uint8_t* audio_file_type, uint32_t audio_duration, uint32_t msg_time);


/**  设置接收语音消息的回调
 * @param pCb 回调函数
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API int32_t txd_file_audio_msg_init(on_receive_audio_msg pCb);


/**  语音消息发送是否成功的回调函数
 * @param err_code 0表示成功，其余见错误码表
 * @param cookie cookie是在调用相应send函数时从出参中获取的，用于标记消息
 */
typedef  void (*on_send_audio_msg_complete)(int32_t err_code, uint32_t cookie);


/**  发送语音文件
 * @note 每秒最多调用该函数1次，否则返回错误码err_msg_send_too_frequently
 * @param audio_file_key 文件的唯一标记
 * @param audio_file_type 语音文件类型，暂只支持"amr"
 * @param audio_duration 语音时长，单位：秒
 * @param pCb 语音消息发送是否成功的回调函数
 * @param pCookie 用于标记该消息，会在回调函数（pCb）中返回
 * @param value 该值匹配命中open.qq.com平台里所配置的触发器后，会触发相应的动作
 *
 * @return 0表示成功，其余见错误码表
 */
SDK_API int32_t txd_file_send_audio_msg(uint8_t* audio_file_key, uint8_t* audio_file_type, uint32_t audio_duration, on_send_audio_msg_complete pCb, uint32_t* pCookie, int32_t value);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TXD_FILE_H__ */
