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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "txd_stdapi.h"

/*****************************************头文件说明******************************************/
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

