/*
 * Copyright (c) 2015 Tencent.
 * All rights reserved.
 */

#ifndef __TXD_STDTYPES_H__
#define __TXD_STDTYPES_H__

#include <stdint.h>

#ifndef SDK_API
#ifdef _x86_
#define SDK_API __attribute__((visibility("default")))
#else
#define SDK_API
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

//针对厂商不支持的标准类型如uint64_t等，厂商统一在此定义

#ifndef bool
#define bool unsigned char
#define true 1
#define false 0
#endif

typedef struct {
    uint8_t buf[8];
} txd_uint64_t;


/**  将txd_uint64_t转换成可打印的字符串，最长为21字节(含末尾的'\0')，该函数在SDK内部实现，厂商可以直接使用这个函数
 * @param u64 入参，txd_uint64_t类型
 * @param str 出参，字符串类型
 */
SDK_API void txd_uint64_to_str(const txd_uint64_t* u64, uint8_t str[21]);

#ifdef _CO_WEIWEN_XT_XCC_
typedef  long long int int64_t;
typedef  unsigned long long uint64_t;
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TXD_STDTYPES_H__ */
