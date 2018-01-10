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

#ifndef __ESP_QQIOT_LOG_H__
#define __ESP_QQIOT_LOG_H__

#include "esp_log.h"
#include "errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log level
 *
 */
typedef enum {
    QQIOT_LOG_NONE,       /*!< No log output */
    QQIOT_LOG_FATAL,      /*!< Fatal error log will output */
    QQIOT_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    QQIOT_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    QQIOT_LOG_INFO,       /*!< Information messages which describe normal flow of events */
    QQIOT_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    QQIOT_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} qqiot_log_level_t;

#ifdef CONFIG_LOG_QQIOT_LEVEL
#define QQIOT_LOCAL_LEVEL CONFIG_LOG_QQIOT_LEVEL
#else
#define QQIOT_LOCAL_LEVEL QQIOT_LOG_NONE
#endif

#if QQIOT_LOCAL_LEVEL >=  QQIOT_LOG_ERROR
#define QQIOT_LOGE( format, ... ) ESP_LOGE(TAG, "[%s, %d]:" format, __func__, __LINE__, ##__VA_ARGS__)
#else
#define QQIOT_LOGE( format, ... )
#endif

#if QQIOT_LOCAL_LEVEL >=  QQIOT_LOG_WARN
#define QQIOT_LOGW( format, ... ) ESP_LOGW(TAG, "[%s, %d]:" format, __func__, __LINE__, ##__VA_ARGS__)
#else
#define QQIOT_LOGW( format, ... )
#endif

#if QQIOT_LOCAL_LEVEL >=  QQIOT_LOG_INFO
#define QQIOT_LOGI( format, ... ) ESP_LOGI(TAG, format, ##__VA_ARGS__)
#else
#define QQIOT_LOGI( format, ... )
#endif

#if QQIOT_LOCAL_LEVEL >=  QQIOT_LOG_DEBUG
#define QQIOT_LOGD( format, ... ) ESP_LOGD(TAG, "[%s, %d]:" format, __func__, __LINE__, ##__VA_ARGS__)
#else
#define QQIOT_LOGD( format, ... )
#endif

#if QQIOT_LOCAL_LEVEL >=  QQIOT_LOG_VERBOSE
#define QQIOT_LOGV( format, ... ) ESP_LOGV(TAG, "[%s, %d]:" format, __func__, __LINE__, ##__VA_ARGS__)
#else
#define QQIOT_LOGV( format, ... )
#endif

/**
 * @brief Check the return value
 */
#define QQIOT_ERROR_CHECK(con, err, format, ...) do { \
        if (con) { \
            QQIOT_LOGE(format , ##__VA_ARGS__); \
            if(errno) QQIOT_LOGE("errno: %d, errno_str: %s\n", errno, strerror(errno)); \
            return err; \
        } \
    } while (0)

/**
 * @brief goto lable
 */
#define QQIOT_ERROR_GOTO(con, lable, format, ...) do { \
        if (con) { \
            QQIOT_LOGE(format , ##__VA_ARGS__); \
            if(errno) QQIOT_LOGE("errno: %d, errno_str: %s\n", errno, strerror(errno)); \
            goto lable; \
        } \
    } while (0)

/**
 * @brief check param
 */
#define QQIOT_PARAM_CHECK(con) do { \
        if (!(con)) { QQIOT_LOGE("parameter error: %s", #con); return ALINK_ERR; } \
    } while (0)

/**
 * @brief Set breakpoint
 */
#define QQIOT_ASSERT(con) do { \
        if (!(con)) { QQIOT_LOGE("errno:%d:%s\n", errno, strerror(errno)); assert(0 && #con); } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif/*!< __ESP_QQIOT_LOG_H__ */


