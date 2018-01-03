/*
 * Copyright (c) 2015 Tencent.
 * All rights reserved.
 */

#ifndef __TXD_QQ_LINK_H__
#define __TXD_QQ_LINK_H__

/*********************************** qqlink接口使用说明 *********************************************/
/*
 * 由于不知道qqlink发送端在那个channel上发包，因此需要在每个可能的channel上抓包，分析，重复这个过程，直到回调通知。
 *
 * step1. txd_qq_link_init初始化qqlink
 * step2. 开启监听模式，并将信道切到下一个信道n（1~13），同时调用txd_qq_link_notify_hop(n), 并在当前信道停留200ms
 * step3. 抓包调用txd_fill_80211_frame函数
 *   step3.1 当txd_fill_80211_frame返回QLERROR_LOCK时，锁定当前信道停留12000ms，转step3
 *   step3.2 当txd_fill_80211_frame返回QLERROR_HOP时，转step2
 *   step3.3 当txd_fill_80211_frame返回QLERROR_SUCCESS时，内部回调用FUNC_NOTIFY函数，返回ssid与pwd用于连接AP，转step4
 * step4. 反初始化调用txd_qq_link_destroy
 */

#include "txd_stdtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

enum wifisyncerror {
    QLERROR_INIT_SUCCESS        = 0,        //suc
    QLERROR_MEMORY_ALLOC  		= 1,        //内存分配失败
    QLERROR_PARAM_KEY       	= 2,        //key传入为NULL
    QLERROR_PARAM_KEY_LEN   	= 3,        //key长度不合法
    QLERROR_INIT_OTHER		    = 4,        //其他错误

};

/*
 * fill_80211_frame的返回值使用此enum定义
 */
enum fill80211relust {
    QLERROR_SUCCESS 		= 0,    //解析成功
    QLERROR_HOP				= 1,    //收到此返回值表示当前信道锁定有误，立马切换下一个信道
    QLERROR_LOCK    		= 2,    //收到此返回值表示是当前信道，锁定

    //下面这些返回值供调试使用，切信道，锁信道根据上面三个值确定,BCAST表示Braodcast，MCAST表示Multicast
    QLERROR_OTHER			= 3,    //其它错误
    QLERROR_DECRYPT_FAILED  = 4,    //解密出错
    QLERROR_NEED_INIT     	= 5,    //qqlink没有初始化
    QLERROR_VERSION        	= 6,    //SDK版本不对应
    QLERROR_START_FRAME     = 7,    //无效的包

    //Broadcast relative
    QLERROR_BCAST_NOT_FRAME	= 8,    //不是广播包
    QLERROR_BCAST_CALC_C	= 9,    //成功计算C值
    QLERROR_BCAST_ONE_DATA	= 10,   //解析到一个广播数据

    //Multicast relative
    QLERROR_MCAST_NOT_FRAME	= 11,   //不是组播包
    QLERROR_MCAST_ONE_DATA	= 12,   //解析到一个组播数据
};

#define QLMAX_SSID_LEN      65
#define QLMAX_PSWD_LEN      65
#define QLMAX_IP_LEN        16

typedef struct {
    int8_t          ssid[QLMAX_SSID_LEN];      // AP账号名称
    int8_t          password[QLMAX_PSWD_LEN];  // AP密码
    int8_t          ip[QLMAX_IP_LEN];          // 发送端IP地址，设备连接AP后反馈给发送端ack时用到
    uint16_t        port;                      // 发送端端口，  设备连接AP后反馈给发送端ack时用到
    uint8_t         bssid[6];                  // AP的MAC地址， 主要用于连接隐藏SSID广播的AP，厂商可根据实际情况选择是否使用，如果设备可以扫描到beacon帧并通过beancon中的信息连接AP则可忽略此成员

} txd_qq_link_result_t;


/**  qq_link完成后的回调，需要厂商自己实现，用来连接路由器
 * @param result 包含路由器SSID和密码信息；回调结束后会被销毁。
 */
SDK_API extern void txd_qq_link_complete(txd_qq_link_result_t* result);


/**  该接口用于初始化数据，如初始化失败之后函数调用都会失败
 * @param key 用于传入解密用的key，一般取设备SN信息，用于解密。该参数不能为NULL也不能传错否则初始化会失败。
 *
 * @return QLERROR_INIT_SUCCESS表示初始化成功
 *         其它表示初始化失败，详见 wifisyncerror
 */
SDK_API int32_t txd_qq_link_init(uint8_t* key);


/**  该函数自动判断并收集数据，当判断收集数据结束时会调用txd_qq_link_complete回调函数，将SSID与PWD传出
 * @param buf 代表网卡抓到的802.11数据包
 * @param len 代表传进来的数据包长度
 * @param offset 代表MPDU偏移位置，即macframe起始位置，根据芯片不同而不同，具体请咨询wifi芯片原厂
 *
 * @return QLERROR_SUCCESS代表解析成功
           QLERROR_HOP代表需要立即切换到下一个信道
           QLERROR_LOCK代表可以锁信道
           其它返回值仅供调试使用
 */
SDK_API int32_t txd_fill_80211_frame(const uint8_t* buf, uint32_t len, int32_t offset);


/**
 * 反初始化 qqlink
 */
void txd_qq_link_destroy();


/**  外部切换信道时通知给sdk内部，做一些清理工作
 * @param channel 当前切换的信道值
 */
void txd_qq_link_notify_hop(uint32_t channel);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // __TXD_QQ_LINK_H__
