/*
 * Copyright (c) 2015 Tencent.
 * All rights reserved.
 */

#ifndef __TXD_STDAPI_H__
#define __TXD_STDAPI_H__

#include "txd_stdtypes.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************头文件说明******************************************/
/*
 * sdk依赖的C库，此文件内的每个接口接入厂商必须实现，否则链接不过,请参考demo
 */
/*********************************************************************************************/

SDK_API extern void txd_memcpy(void* dest, const void* src, uint32_t n);

SDK_API extern void txd_memset(void* p, int c, uint32_t n);

SDK_API extern int32_t txd_memcmp(const void* p1, const void* p2, uint32_t n);

SDK_API extern uint32_t txd_strlen(const char* s);

SDK_API extern int32_t txd_atoi(const char* s);

SDK_API extern int32_t txd_vsnprintf(char* s, uint32_t size, const char* template, va_list ap);

SDK_API extern int32_t txd_printf(const char* template, ...);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TXD_STDAPI_H__ */
