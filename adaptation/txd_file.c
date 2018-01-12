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

/*
 * 文件操作接口
 */
struct txd_file_handler_t {
    FILE* fp;
    txd_file_info_t info;
};

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
        return file;
    }

    memset(file, 0x0, sizeof(txd_file_handler_t));

    if (file_type) {
        file->info.file_type_len = txd_strlen((char *)file_type);

        if (file->info.file_type_len > sizeof(file->info.file_type)) {
            file->info.file_type_len = sizeof(file->info.file_type);
        }

        strncpy((char *)(file->info.file_type), (char *)file_type, file->info.file_type_len);
    }

    if (!file_key || txd_strlen((char*)file_key) == 0) {
#define TEMP_FILE_KEY_LENGTH 8
        temp_file_key = (uint8_t*)txd_malloc((TEMP_FILE_KEY_LENGTH + 1) * sizeof(uint8_t));

        if (temp_file_key == NULL) {
            txd_free(file);
            return NULL;
        }

        do {
            count--;

            if (count <= 0) {
                txd_free(file);
                txd_free(temp_file_key);
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

                strncpy((char *)(file->info.file_key), (char *)temp_file_key, file->info.file_key_len);
                txd_free(temp_file_key);
                break;
            }
        } while (1);
    } else {
        file->info.file_key_len = txd_strlen((char *)(file->info.file_key));

        if (file->info.file_key_len > sizeof(file->info.file_key)) {
            file->info.file_key_len = sizeof(file->info.file_key);
        }

        strncpy((char *)(file->info.file_key), (char *)file_key, file->info.file_key_len);
    }

    filename = (char*)txd_malloc(file->info.file_key_len + 1);

    if (!filename) {
        txd_free(file);
        return NULL;
    }

    memcpy(filename, file->info.file_key, file->info.file_key_len);

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
            return NULL;
        }

        file->info.file_size = statbuf.st_size;
    } else {
        txd_free(file);
        txd_free(filename);
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
        return ret;
    }

    filename = (char*)txd_malloc(file->info.file_key_len + 1);

    if (!filename) {
        return ret;
    }

    memcpy(filename, file->info.file_key, file->info.file_key_len);

    struct stat statbuf;

    if (stat(filename, &statbuf) < 0) {
        txd_free(filename);
        return ret;
    }

    txd_free(filename);

    info->file_size = statbuf.st_size;
    memcpy(info, &file->info, sizeof(*info));

    ret = 0;
    return ret;
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
    char* filename = (char*)txd_malloc(txd_strlen((char*)file_key) + 1);

    if (!filename) {
        return ret;
    }

    ret = remove(filename);
    txd_free(filename);
    return ret;
}

