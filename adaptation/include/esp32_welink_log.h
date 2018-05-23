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

#ifndef __ESP_WELINK_LOG_H__
#define __ESP_WELINK_LOG_H__

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
    WELINK_LOG_NONE,       /*!< No log output */
    WELINK_LOG_FATAL,      /*!< Fatal error log will output */
    WELINK_LOG_ERROR,      /*!< Critical errors, software module can not recover on its own */
    WELINK_LOG_WARN,       /*!< Error conditions from which recovery measures have been taken */
    WELINK_LOG_INFO,       /*!< Information messages which describe normal flow of events */
    WELINK_LOG_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    WELINK_LOG_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} WELINK_log_level_t;

#ifdef CONFIG_LOG_WELINK_LEVEL
#define WELINK_LOCAL_LEVEL CONFIG_LOG_WELINK_LEVEL
#else
#define WELINK_LOCAL_LEVEL WELINK_LOG_NONE
#endif

#if WELINK_LOCAL_LEVEL >=  WELINK_LOG_ERROR
#define WELINK_LOGE( format, ... ) ESP_LOGE(TAG, "[%s, %d]:" format, __func__, __LINE__, ##__VA_ARGS__)
#else
#define WELINK_LOGE( format, ... )
#endif

#if WELINK_LOCAL_LEVEL >=  WELINK_LOG_WARN
#define WELINK_LOGW( format, ... ) ESP_LOGW(TAG, "[%s, %d]:" format, __func__, __LINE__, ##__VA_ARGS__)
#else
#define WELINK_LOGW( format, ... )
#endif

#if WELINK_LOCAL_LEVEL >=  WELINK_LOG_INFO
#define WELINK_LOGI( format, ... ) ESP_LOGI(TAG, format, ##__VA_ARGS__)
#else
#define WELINK_LOGI( format, ... )
#endif

#if WELINK_LOCAL_LEVEL >=  WELINK_LOG_DEBUG
#define WELINK_LOGD( format, ... ) ESP_LOGD(TAG, "[%s, %d]:" format, __func__, __LINE__, ##__VA_ARGS__)
#else
#define WELINK_LOGD( format, ... )
#endif

#if WELINK_LOCAL_LEVEL >=  WELINK_LOG_VERBOSE
#define WELINK_LOGV( format, ... ) ESP_LOGV(TAG, "[%s, %d]:" format, __func__, __LINE__, ##__VA_ARGS__)
#else
#define WELINK_LOGV( format, ... )
#endif

/**
 * @brief Check the return value
 */
#define WELINK_ERROR_CHECK(con, err, format, ...) do { \
        if (con) { \
            WELINK_LOGE(format , ##__VA_ARGS__); \
            if(errno) WELINK_LOGE("errno: %d, errno_str: %s\n", errno, strerror(errno)); \
            return err; \
        } \
    } while (0)

/**
 * @brief goto lable
 */
#define WELINK_ERROR_GOTO(con, lable, format, ...) do { \
        if (con) { \
            WELINK_LOGE(format , ##__VA_ARGS__); \
            if(errno) WELINK_LOGE("errno: %d, errno_str: %s\n", errno, strerror(errno)); \
            goto lable; \
        } \
    } while (0)

/**
 * @brief check param
 */
#define WELINK_PARAM_CHECK(con) do { \
        if (!(con)) { WELINK_LOGE("parameter error: %s", #con); return ALINK_ERR; } \
    } while (0)

/**
 * @brief Set breakpoint
 */
#define WELINK_ASSERT(con) do { \
        if (!(con)) { WELINK_LOGE("errno:%d:%s\n", errno, strerror(errno)); assert(0 && #con); } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif/*!< __ESP_WELINK_LOG_H__ */


