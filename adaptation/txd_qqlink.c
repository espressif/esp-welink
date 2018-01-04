/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
#include "esp_wifi.h"

#include "txd_stdapi.h"
#include "txd_baseapi.h"
#include "txd_qq_link.h"

/**
*  设置当前抓包的频道，由厂商实现（注：不一定非要在此函数返回前切换频道，也可以设置值在其他地方检测到改变再调频）
*  nchannel ： 需要跳到的频道数值，厂商根据该数值跳到相应的频道
*/
void txd_set_qq_link_channel(int32_t nchannel)
{
    static wifi_second_chan_t second_ch = WIFI_SECOND_CHAN_NONE; //0,1,2
    esp_wifi_set_channel(nchannel, second_ch);
}
/**
*  qq_link 完成后回调,需要厂商自己实现，主要是连接路由器
*  pqq_link_param     :   包含路由器SSID和密码信息；回调结束后会被销毁。
*/
void txd_on_qq_link_notify(txd_qq_link_result_t* pqq_link_param)
{
    wifi_config_t wifi_config;

    if ((pqq_link_param == NULL) || (pqq_link_param->ssid == NULL)) {
        return ;
    }

    memset(&wifi_config, 0, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, pqq_link_param->ssid, txd_strlen((char*)(pqq_link_param->ssid)));
    memcpy(wifi_config.sta.password, pqq_link_param->password, txd_strlen((char*)(pqq_link_param->password)));

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_connect();
}
