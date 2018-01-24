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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "txd_baseapi.h"
#include "txd_thread.h"
#include "esp_qqiot_log.h"

static const char* TAG = "txd_thread";

struct txd_thread_handler_t {
    TaskHandle_t xHandle;
    txd_thread_callback txd_thread_cb;
};

struct txd_mutex_handler_t {
    SemaphoreHandle_t xHandle;
};

/************************ thread 接口 *********************************/
/**  创建线程
 * @param priority SDK若期望使用系统默认值， 则在调用时会传入0
 * @param stack_size 线程需要使用的栈大小
 * @param callback 线程的执行函数体
 * @param arg callback的参数
 *
 * @return 线程（句柄）标记
 */
txd_thread_handler_t* txd_thread_create(uint8_t priority,
                                        uint32_t stack_size,
                                        txd_thread_callback callback,
                                        void* arg)
{
    txd_thread_handler_t* threader_hd = (txd_thread_handler_t*)txd_malloc(sizeof(txd_thread_handler_t));

    if (threader_hd == NULL) {
        QQIOT_LOGE("malloc fail");
        return threader_hd;
    }

    memset(threader_hd, 0, sizeof(txd_thread_handler_t));
    threader_hd->txd_thread_cb = callback;

    if (xTaskCreate(callback, "qq_iot_task", stack_size / sizeof(portSTACK_TYPE), arg, priority, &(threader_hd->xHandle)) != pdTRUE) {
        txd_free(threader_hd);
        threader_hd = NULL;
        QQIOT_LOGE("thread create fail");
    }

    return threader_hd;
}

/**  销毁线程
 * @param thread 线程（句柄）标记
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
int32_t txd_thread_destroy(txd_thread_handler_t* thread)
{
    int32_t ret = -1;

    if (thread == NULL) {
        QQIOT_LOGE("the parameter is incorrect");
        return ret;
    }

    vTaskDelete(thread->xHandle);
    txd_free(thread);
    return 0;
}

/************************ mutex 接口 *********************************/
/**  创建mutex
 *
 * @return mutex
 */
txd_mutex_handler_t* txd_mutex_create()
{
    txd_mutex_handler_t* mutex = (txd_mutex_handler_t*)txd_malloc(sizeof(txd_mutex_handler_t));

    if (mutex == NULL) {
        QQIOT_LOGE("malloc fail");
        return mutex;
    }

    mutex->xHandle = xSemaphoreCreateMutex();

    if (mutex->xHandle == NULL) {
        txd_free(mutex);
        mutex = NULL;
        QQIOT_LOGE("create Mutex fail");
    }

    return mutex;
}

/**  锁住mutex
 * @param mutex
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
int32_t txd_mutex_lock(txd_mutex_handler_t* mutex)
{
    int32_t ret = -1;

    if ((mutex == NULL) || (mutex->xHandle == NULL)) {
        QQIOT_LOGE("the parameter is incorrect");
        return ret;
    }

    if (xSemaphoreTake(mutex->xHandle, portMAX_DELAY) == pdTRUE) {
        QQIOT_LOGI("Mutex lock");
        ret = 0;
    }

    return ret;
}

/**  解锁mutex
 * @param mutex
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
int32_t txd_mutex_unlock(txd_mutex_handler_t* mutex)
{
    int32_t ret = -1;

    if ((mutex == NULL) || (mutex->xHandle == NULL)) {
        QQIOT_LOGE("the parameter is incorrect");
        return ret;
    }

    if (xSemaphoreGive(mutex->xHandle) == pdTRUE) {
        QQIOT_LOGI("Mutex unlock");
        ret = 0;
    }

    return ret;
}

/**  销毁mutex
 * @param mutex
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
int32_t txd_mutex_destroy(txd_mutex_handler_t* mutex)
{
    if ((mutex == NULL) || (mutex->xHandle == NULL)) {
        QQIOT_LOGE("the parameter is incorrect");
        return -1;
    }

    vSemaphoreDelete(mutex->xHandle);
    txd_free(mutex);
    return 0;
}

/************************** sleep接口 接入厂商实现*****************************/
/*
 * sleep当前线程，不需要指定线程id，跟linux下的sleep功能（用法）一样，注意这里的单位是ms
 */
int32_t txd_sleep(uint32_t milliseconds)
{
    vTaskDelay(milliseconds / (1000 / xPortGetTickRateHz()));
    return 0;
}
