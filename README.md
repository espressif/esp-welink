# ESP-Welink-Demo  

## 1. 腾讯微瓴简介

![](https://cdn.weihome.qq.com/open-platform/static/img/banner.d531c15.png)

腾讯微瓴是一个适合行业的, 安全灵活且可以高效触达用户的物联网云平台, 开发者可以基于微瓴进行软硬件开发.  
腾讯微瓴物联网套件提供以下软硬件服务：

> - 兼容常规物联网通信协议: ZigBee, Wi-Fi等;
> - 多种硬件平台支持, 兼容Android, Linux, RTOS等操作系统;
> - 支持多达6个专业设备类型, 支持多达70种公共协议指令;
> - 高效便捷的开发调试平台工具, 完备详尽的开发者文档和范例代码.
> - 支持多达6种物联设备, 10种设备指令的调用;
> - 丰富的AI算法和引擎的调用: 如人脸识别, 失物追踪等等;
> - 丰富的业务系统模块和服务调用: 如智能照明, 设备管理等等;
> - 高效便捷的开发调试平台工具, 完备详尽的开发者文档和范例代码.

关于腾讯微瓴的详细介绍资料, 请参考[腾讯微瓴开放平台](https://open.welink.qq.com/)

## 2. 硬件平台简介

此 Demo 同时支持 ESP32 和 ESP8266.

- ESP32-DevKitC 模组如下图所示:

![](https://www.espressif.com/sites/default/files/dev-board/ESP32-DevKitC-32D-F_01_0.jpg)

- 模组详细资料请参考：[ESP32-DevKitC 模组](http://esp-idf.readthedocs.io/en/latest/hw-reference/modules-and-boards.html#esp32-core-board-v2-esp32-devkitc) 或 [ESP-WROVER-KIT 模组](http://esp-idf.readthedocs.io/en/latest/hw-reference/modules-and-boards.html#esp-wrover-kit)

- ESP8266-DevKitC 模组如下图所示:

![](https://www.espressif.com/sites/default/files/dev-board/ESP8266-DevKitC-02D-F-01.jpg)

- 模组详细资料请参考：[ESP8266-DevKitC 模组](https://www.espressif.com/en/products/hardware/development-boards)

- 路由器/ Wi-Fi 热点：可以连接外网

## 3. esp-welink 下载

- Demo下载[链接](https://github.com/espressif/esp-welink)

- Demo目录结构如下所示:

```
esp-welink
├── build                                   //存放编译后生成的文件
├── examples
│   └── sdk-demo                            //demo 文件夹
│       ├── main
│       │   ├── component.mk
│       │   ├── Kconfig.projbuild
│       │   ├── main.c                      //入口文件
│       │   ├── txd_welink.c
│       │   ├── txd_welink.h
│       │   ├── txd_wifi.c
│       │   └── txd_wifi.h
│       ├── Makefile                        //编译入口makefile
│       ├── partitions_welink_demo.csv      //分区配置文件
│       ├── README.md
│       └── sdkconfig.defaults              //默认menuconfig配置文件
├── port                                    //welink 适配层
│   ├── component.mk
│   ├── include
│   │   └── esp_welink_log.h
│   ├── txd_baseapi.c
│   ├── txd_stdapi.c
│   └── txd_thread.c
├── component.mk
├── README.md
└── welink                                  //welink sdk
    ├── component.mk
    ├── include
    │   ├── txd_baseapi.h
    │   ├── txd_error.h
    │   ├── txd_ota.h
    │   ├── txd_sdk.h
    │   ├── txd_stdapi.h
    │   ├── txd_stdtypes.h
    │   └── txd_thread.h
    ├── lib
    │   ├── libtxdevicesdk32.a
    │   └── libtxdevicesdk8266.a
    └── README.md
```

- Demo下载步骤

如果之前您没有搭建过esp的开发环境, ESP32 请先参考esp-idf[README.md](https://github.com/espressif/esp-idf), ESP8266 请参考 ESP8266_RTOS_SDK
 [README.md](https://github.com/espressif/ESP8266_RTOS_SDK/blob/master/README.md).

```
mkdir -p ~/esp
cd ~/esp
git clone https://github.com/espressif/esp-welink
cd esp-welink
make
```
- 工程配置

在编译工程之前, 您还需要修改demo的相关参数, 这些参数取决于您的测试环境以及测试帐号:
  1. 测试路由器SSID, PASSWORD
  2. PN, SN, Licesne
  3. CLIENT_PUB_KEY, AUTH_KEY
  
以上参数, 除了`CLIENT_PUB_KEY`和`AUTH_KEY`,需要在Demo的代码中(esp-welink/examples/sdk_demo/main/txd_welink.c)改, 其余的都在menuconfig中改

```
make menuconfig -> Welink Configuration
```

## 4. Welink Demo运行与调试

设置好参数之后, 可以编译工程并下载到模组里:

```
make
make erase_flash
make flash monitor
```

当代码烧录进模组之后, 会连接到您配置的路由器上, 然后sdk自动连接welink云端, 您可以在welink[调试页面](https://open.welink.qq.com/#/device)调试模组与welink云端的通信.

![](https://cdn.weihome.qq.com/open-platform/static/img/beginner-5-1-1.c32fe11.png)

详细调试介绍文档, 请参考[腾讯微瓴开放平台](https://open.welink.qq.com/)

