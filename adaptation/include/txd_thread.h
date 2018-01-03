/*
 * Copyright (c) 2015 Tencent.
 * All rights reserved.
 */

#ifndef __TXD_THREAD_H__
#define __TXD_THREAD_H__

#include "txd_stdtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************* thread 接口 **********************************/

typedef struct txd_thread_handler_t txd_thread_handler_t;

typedef void (*txd_thread_callback)(void *);


/**  创建线程
 * @param priority SDK若期望使用系统默认值， 则在调用时会传入0
 * @param stack_size 线程需要使用的栈大小
 * @param callback 线程的执行函数体
 * @param arg callback的参数
 *
 * @return 线程（句柄）标记
 */
SDK_API extern txd_thread_handler_t* txd_thread_create(uint8_t priority,
                                uint32_t stack_size,
                                txd_thread_callback callback,
                                void* arg);


/**  销毁线程
 * @param thread 线程（句柄）标记
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_thread_destroy(txd_thread_handler_t *thread);



/************************* mutex 接口 **********************************/
typedef struct txd_mutex_handler_t txd_mutex_handler_t;


/**  创建mutex
 *
 * @return mutex
 */
SDK_API extern txd_mutex_handler_t* txd_mutex_create();


/**  锁住mutex
 * @param mutex
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_mutex_lock(txd_mutex_handler_t *mutex);


/**  解锁mutex
 * @param mutex
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_mutex_unlock(txd_mutex_handler_t *mutex);


/**  销毁mutex
 * @param mutex
 *
 * @return 0 表示成功
 *         -1 表示失败
 */
SDK_API extern int32_t txd_mutex_destroy(txd_mutex_handler_t *mutex);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TXD_THREAD_H__ */
