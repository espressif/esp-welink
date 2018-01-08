/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
    esp_wifi_set_channel(nchannel, WIFI_SECOND_CHAN_NONE);
}

/**
**  qq_link完成后的回调，需要厂商自己实现，用来连接路由器
* @param result 包含路由器SSID和密码信息；回调结束后会被销毁。
*/
void txd_qq_link_complete(txd_qq_link_result_t* result)
{
    wifi_config_t wifi_config;

    if ((result == NULL) || (result->ssid == NULL)) {
        return ;
    }

    memset(&wifi_config, 0, sizeof(wifi_config_t));

    if ((txd_strlen((char*)(result->ssid)) > sizeof(wifi_config.sta.ssid)) || (txd_strlen((char*)(result->password)) > sizeof(wifi_config.sta.password))) {
        return;
    }

    memcpy(wifi_config.sta.ssid, result->ssid, txd_strlen((char*)(result->ssid)));
    memcpy(wifi_config.sta.password, result->password, txd_strlen((char*)(result->password)));

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_connect();
}
