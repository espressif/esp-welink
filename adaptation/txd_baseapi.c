/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP32 only, in which case,
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <sys/time.h>

#include "esp_log.h"
#include "txd_stdapi.h"

/******************************************头文件说明******************************************/
/*
 * sdk依赖的C库，此文件内的每个接口接入厂商必须实现，否则链接不过,请参考demo
 */
/*********************************************************************************************/

void txd_memcpy(void* dest, const void* src, uint32_t n)
{
    memcpy(dest, src, n);
}

void txd_memset(void* p, int c, uint32_t n)
{
    memset(p, c, n);
}

int32_t txd_memcmp(const void* p1, const void* p2, uint32_t n)
{
    return memcmp(p1, p2, n);
}

uint32_t txd_strlen(const char* s)
{
    return strlen(s);
}

int32_t txd_atoi(const char* s)
{
    return atoi(s);
}

int32_t txd_vsnprintf(char* s, uint32_t size, const char* template, va_list ap)
{
    return vsnprintf(s, size, template, ap);
}

int32_t txd_printf(const char* template, ...)
{
    int32_t ret = 0;

    va_list args;
    va_start(args, template);
    ret = vprintf(template, args);
    va_end(args);
    return ret;
}

/************************ memory接口 接入厂商实现*********************************/
/*
 * 内存申请与释放接口，这样SDK使用者可以做一些内存池相关的策略
 * 目前这里开发者可以直接使用C的malloc与free替代，此接口很少使用后续考虑删除
 */
/*
 * 申请内存，同malloc
 */
void* txd_malloc(uint32_t size)
{
    return malloc(size);
}

/*
 * 释放内存，同free
 */
void txd_free(void* p)
{
    free(p);
}

/************************ time接口 接入厂商实现*********************************/
/*
 * 获得当前本地系统时间，返回从1970年开始的[秒数]
 * 注意：这里单位是秒
 */
uint32_t txd_time_get_local()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (tv.tv_sec + tv.tv_usec / 1000000);
}

/*
 * 获得当前系统时钟时间，相对开机时的系统时钟[毫秒数]
 * 将超过uint32_t的部分截断即可（SDK内部会维护溢出部分）
 * 注意：这里单位是毫秒
 */
uint32_t txd_time_get_sysclock()
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return esp_log_early_timestamp();
    }

    static uint32_t base = 0;

    if (base == 0 && xPortGetCoreID() == 0) {
        base = esp_log_early_timestamp();
    }

    return base + xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
}

