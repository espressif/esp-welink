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

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "esp_system.h"

#include "txd_stdapi.h"
#include "txd_file.h"
#include "txd_baseapi.h"
#include "esp_qqiot_log.h"

static const char* TAG = "txd_file";
#define ESP_FATFS_BASE_PATH "/fatfs"
/*
 * 文件操作接口
 */
struct txd_file_handler_t {
    FILE* fp;
    txd_file_info_t info;
};

/************************** store接口 接入厂商实现*****************************/
/*
 * 持久化数据时，SDK不关心具体写到哪一个文件，
 * 但是要保证在读取的时候能够读取到相应的数据。
 * 建议每个配对的读写函数对应不同的文件。
 * 目前这里读写本地存储1k，开发者在flash里开辟一块1k的存储即可
 */

/*
 * 将设备的基础信息持久化，即将buf指针所指的内存写入count个字节到文件中，
 * 返回实际写入的字节数，有错误发生则返回-1
 */
int32_t txd_write_basicinfo(uint8_t* buf, uint32_t count)
{
    int32_t ret = -1;
    char* filename = (char*)txd_malloc(strlen(ESP_FATFS_BASE_PATH) + strlen("esp32_tencent_basicinfo") + 2);

    if (filename == NULL) {
        QQIOT_LOGE("malloc fail");
        return ret;
    }

    sprintf((char*)filename, "%s/%s", ESP_FATFS_BASE_PATH, "esp32_tencent_basicinfo");
    FILE* f = fopen(filename, "wb");

    if (f == NULL) {
        QQIOT_LOGE("fopen fail");
        txd_free(filename);
        return ret;
    }

    ret = fwrite(buf, 1, count, f);
    fclose(f);
    txd_free(filename);
    return ret;
}

/*
 * 读取已经持久化的设备基础信息，读取count个字节到buf指针所指的内存中，
 * 返回实际读入的字节数，有错误发生则返回-1
 */
int32_t txd_read_basicinfo(uint8_t* buf, uint32_t count)
{
    int32_t ret = -1;
    char* filename = (char*)txd_malloc(strlen(ESP_FATFS_BASE_PATH) + strlen("esp32_tencent_basicinfo") + 2);

    if (filename == NULL) {
        QQIOT_LOGE("malloc fail");
        return ret;
    }

    sprintf((char*)filename, "%s/%s", ESP_FATFS_BASE_PATH, "esp32_tencent_basicinfo");

    FILE* f = fopen(filename, "rb");

    if (f == NULL) {
        QQIOT_LOGE("fopen fail");
        txd_free(filename);
        return ret;
    }

    ret = fread(buf, 1, count, f);
    txd_free(filename);
    fclose(f);
    return ret;
}

/************************ file 接口 *********************************/
/*
 * 接口说明：以mode方式打开文件，文件类型为file_type
 * 参数说明: file_type:文件类型，'\0'结尾字符串，即文件后缀，如"amr"
            file_key:以'\0'结尾的字符串，如果filekey为空，函数内部应该自己生成filekey
            mode：用于任务配对的任务id，在相应的回调函数中会携带
 */
txd_file_handler_t* txd_file_open(uint8_t* file_type, uint8_t* file_key, txd_file_mode_t mode)
{
    uint8_t* temp_file_key = NULL;
    char* filename = NULL;
    int32_t count = 100;

    txd_file_handler_t* file = (txd_file_handler_t*)txd_malloc(sizeof(txd_file_handler_t));

    if (!file) {
        QQIOT_LOGE("malloc fail");
        return file;
    }

    memset(file, 0x0, sizeof(txd_file_handler_t));

    if (file_type) {
        file->info.file_type_len = txd_strlen((char*)file_type);

        if (file->info.file_type_len > sizeof(file->info.file_type)) {
            file->info.file_type_len = sizeof(file->info.file_type);
        }

        strncpy((char*)(file->info.file_type), (char*)file_type, file->info.file_type_len);
    }

    if (!file_key || txd_strlen((char*)file_key) == 0) {
        QQIOT_LOGD("the file_key is empty, need to generate it internal");
#define TEMP_FILE_KEY_LENGTH 8
        temp_file_key = (uint8_t*)txd_malloc((TEMP_FILE_KEY_LENGTH + 1) * sizeof(uint8_t));

        if (temp_file_key == NULL) {
            txd_free(file);
            QQIOT_LOGE("malloc fail");
            return NULL;
        }

        do {
            count--;

            if (count <= 0) {
                txd_free(file);
                txd_free(temp_file_key);
                QQIOT_LOGE("generate file_key timeout");
                return NULL;
            }

            for (int i = 0; i < TEMP_FILE_KEY_LENGTH; i++) {
                temp_file_key[i] = ('A' + esp_random() % 26);
            }

            temp_file_key[TEMP_FILE_KEY_LENGTH] = '\0';

            if (access((char*)temp_file_key, 0) == -1) {
                file->info.file_key_len = txd_strlen((char*)temp_file_key);

                if (file->info.file_key_len > sizeof(file->info.file_key)) {
                    file->info.file_key_len = sizeof(file->info.file_key);
                }

                strncpy((char*)(file->info.file_key), (char*)temp_file_key, file->info.file_key_len);
                txd_free(temp_file_key);
                QQIOT_LOGD("generate file_key success: %s", (char*)(file->info.file_key));
                break;
            }
        } while (1);
    } else {
        file->info.file_key_len = txd_strlen((char*)(file->info.file_key));

        if (file->info.file_key_len > sizeof(file->info.file_key)) {
            file->info.file_key_len = sizeof(file->info.file_key);
        }

        strncpy((char*)(file->info.file_key), (char*)file_key, file->info.file_key_len);
    }

    filename = (char*)txd_malloc(file->info.file_key_len + strlen(ESP_FATFS_BASE_PATH) + 2);

    if (!filename) {
        txd_free(file);
        QQIOT_LOGE("malloc fail");
        return NULL;
    }

    sprintf((char*)filename, "%s/%s", ESP_FATFS_BASE_PATH, file->info.file_key);

    switch (mode) {
        case fmode_r:
            file->fp = fopen(filename, "rb");
            break;

        case fmode_w:
            file->fp = fopen(filename, "wb");
            break;

        default:
            file->fp = NULL;
            break;
    }

    if (file->fp) {
        struct stat statbuf;

        if (stat(filename, &statbuf) < 0) {
            txd_free(file);
            txd_free(filename);
            QQIOT_LOGE("get file information fail");
            return NULL;
        }

        file->info.file_size = statbuf.st_size;
    } else {
        txd_free(file);
        txd_free(filename);
        QQIOT_LOGE("file->fp is NULL");
        return NULL;
    }

    txd_free(filename);
    return file;
}

/*
 * 从file中读取数据到buf[0:n)中
 * 返回值为-1：表示读取发生错误
 * 返回值为0：表示已经读取完毕
 * 返回值为正数：表示读取到的字节数
 */
int32_t txd_file_read(txd_file_handler_t* file, uint8_t* buf, uint32_t n)
{
    int32_t realRead = -1;

    if ((file == NULL) || (buf == NULL) || (txd_strlen((char*)(file->info.file_key)) == 0) || (file->fp == NULL)) {
        QQIOT_LOGE("the parameter is incorrect");
        return realRead;
    }

    realRead = fread(buf, 1, n, file->fp);

    return realRead;
}

/*
 * 向file中写入buf[0:n)
 * 返回值为-1：表示写入发生错误
 * 返回值为0：表示没有写入数据
 * 返回值为正数：表示写入的字节数
 */
int32_t txd_file_write(txd_file_handler_t* file, uint8_t* buf, uint32_t n)
{
    int32_t realWrite = -1;

    if ((file == NULL) || (buf == NULL) || (txd_strlen((char*)(file->info.file_key)) == 0) || (file->fp == NULL)) {
        QQIOT_LOGE("the parameter is incorrect");
        return realWrite;
    }

    realWrite = fwrite(buf, 1, n , file->fp);

    return realWrite;
}

/*
 * 获取file的基本属性
 * 返回值为-1：表示获取file信息失败
 * 返回值为0：表示获取file信息成功
 */
int32_t txd_file_get_info(txd_file_handler_t* file, txd_file_info_t* info)
{
    int32_t ret = -1;
    char* filename = NULL;

    if ((file == NULL) || (info == NULL) || (file->info.file_key == NULL)) {
        QQIOT_LOGE("the parameter is incorrect");
        return ret;
    }

    filename = (char*)txd_malloc(file->info.file_key_len + strlen(ESP_FATFS_BASE_PATH) + 2);

    if (!filename) {
        QQIOT_LOGE("malloc fail");
        return ret;
    }

    sprintf((char*)filename, "%s/%s", ESP_FATFS_BASE_PATH, file->info.file_key);

    struct stat statbuf;

    if (stat(filename, &statbuf) < 0) {
        txd_free(filename);
        QQIOT_LOGE("get file information fail");
        return ret;
    }

    txd_free(filename);

    info->file_size = statbuf.st_size;
    memcpy(info, &file->info, sizeof(*info));

    return 0;
}

/*
 * 移动文件读写指针到文件的第pos个字节
 * pos为0：表示指向文件头部，也就是刚打开文件的状态
 * pos为0xffffffff：表示指向文件末尾，无需知道文件的大小
 */
int32_t txd_file_seek(txd_file_handler_t* file, uint32_t pos)
{
    int32_t ret = -1;

    if ((file == NULL) || (file->fp == NULL)) {
        QQIOT_LOGE("the parameter is incorrect");
        return ret;
    }

    switch (pos) {
        case 0:
            ret = fseek(file->fp, 0, SEEK_SET);
            break;

        case 0xffffffff:
            ret = fseek(file->fp, 0, SEEK_END);
            break;

        default:
            ret = fseek(file->fp, pos, SEEK_SET);
            break;
    }

    return ret;
}

/*
 * 关闭文件
 */
int32_t txd_file_close(txd_file_handler_t* file)
{
    int32_t ret = -1;

    if ((file == NULL) || (file->fp == NULL)) {
        QQIOT_LOGE("the parameter is incorrect");
        return ret;
    }

    ret = fclose(file->fp);
    txd_free(file);
    return ret;
}

/**  删除文件
 * @note SDK在下载完文件后会对文件做校验，如果校验失败，则会调用txd_file_remove将其删除
 *       如果该文件已经被打开，SDK会先调用txd_file_close将其关闭，然后再调用txd_file_remove
 * @param file_key 唯一标记该文件，以'\0'结尾的字符串
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
int32_t txd_file_remove(uint8_t* file_key)
{
    int32_t ret = -1;
    char* filename = NULL;

    if (!file_key) {
        QQIOT_LOGE("the parameter is incorrect");
        return ret;
    }

    filename = (char*)txd_malloc(file->info.file_key_len + strlen(ESP_FATFS_BASE_PATH) + 2);

    if (!filename) {
        QQIOT_LOGE("malloc fail");
        return ret;
    }

    sprintf((char*)filename, "%s/%s", ESP_FATFS_BASE_PATH, file->info.file_key);
    ret = remove(filename);
    txd_free(filename);
    return ret;
}

