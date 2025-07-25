# 智能温室控制系统

本项目基于 STM32F103ZET6 微控制器，集成了温湿度、光照等多种环境监测与自动控制功能，适用于智能温室、环境实验箱等场景。系统支持本地显示、按键交互、数据记录、蓝牙通信等多种功能，便于远程监控与智能管理。

## 主要功能

- **环境监测**：实时采集温度、湿度（DHT11）、光照强度（光敏电阻/ADC）。
- **自动/手动控制**：支持自动与手动两种工作模式，可智能或手动控制风扇、水泵、补光灯等设备。
- **多级报警**：支持高温、低温、高湿、低湿、光照不足、传感器故障等多种报警类型。
- **数据记录**：环境数据、操作记录、报警事件等均可存储于片上 Flash，支持历史查询与统计。
- **本地显示**：TFT LCD 实时显示环境参数、设备状态、报警信息等。
- **RGB LED 矩阵**：5x5 WS2812 RGB 彩灯阵列，用于状态/动画/图案显示，提升交互体验。
- **按键交互**：多功能按键支持模式切换、设备手动控制等。
- **蓝牙通信**：集成 HC05 蓝牙模块，支持手机 App 远程监控与控制。
- **串口通信**：支持 USART1/USART3，便于调试与数据输出。

## 系统结构

```
├── APP/                  // 应用层功能模块
│   ├── beep/             // 蜂鸣器报警
│   ├── config/           // 配置参数管理
│   ├── data_logger/      // 数据记录与查询
│   ├── dht11/            // 温湿度传感器
│   ├── fan_pwm/          // 风扇PWM控制
│   ├── greenhouse_control/ // 温室主控与显示
│   ├── hc05/             // 蓝牙通信
│   ├── key/              // 按键扫描
│   ├── led/              // 指示灯
│   ├── lsens/            // 光照传感器
│   ├── rtc/              // 实时时钟
│   ├── tftlcd/           // LCD显示驱动
│   └── ws2812/           // RGB LED矩阵
├── Libraries/            // STM32标准外设库
├── Public/               // 公共驱动与工具
├── User/                 // 用户主程序、启动文件
```

## 主要硬件连接

- DHT11温湿度：PG11
- 光照传感器：PF8 (ADC3_IN6)
- 系统LED：PB5
- 补光LED：PE5、PC6
- 风扇PWM：PA6 (TIM3_CH1)
- 水泵LED：PC8
- 蜂鸣器：PB8
- 按键：PE2、PE3、PE4、PA0
- RGB矩阵：PE6 (WS2812)
- 蓝牙HC05：PB10/PB11（USART3），KEY-PA4，LED-PA15

## 主要软件功能说明

### 1. 传感器与自动控制
- 定时采集温湿度、光照数据，自动判断是否需要开启/关闭风扇、水泵、补光灯。
- 支持滞回、简单开关、预留PID等多种控制算法。
- 支持手动强制控制各设备。

### 2. 数据记录与查询
- 采用片上 Flash 存储，支持传感器数据、操作、报警等多类型日志。
- 支持按时间、类型等条件查询，便于历史追溯与统计分析。

### 3. 显示与交互
- TFT LCD 实时显示环境参数、设备状态、报警信息等。
- 5x5 RGB LED阵列可显示温湿度、光照、系统状态、动画、图案等。
- 多按键支持模式切换、设备手动控制等。

### 4. 通信接口
- 串口1（USART1）：用于PC调试、数据输出。
- 串口3（USART3）：连接HC05蓝牙模块，支持手机App远程控制。
- 蓝牙命令支持状态查询、模式切换、设备控制、动画显示等。

### 5. 报警与提示
- 多级报警（高温、低温、高湿、低湿、光照不足、传感器故障），蜂鸣器、LED、LCD、RGB矩阵多重提示。

## 快速上手

1. 使用 Keil MDK 打开 `GreenhouseControl.uvprojx` 工程文件。
2. 选择目标板为 STM32F103ZET6，编译并下载固件至开发板。
3. 连接传感器、执行板载硬件连线。
4. 通过串口或蓝牙连接，使用串口调试助手或手机App（如“Serial Bluetooth Terminal”）进行交互。
5. 按照 `蓝牙通信实时测试指南.txt` 进行蓝牙配对与命令测试。

## 蓝牙命令示例

- `STATUS`：查询当前环境与设备状态
- `MANUAL`/`AUTO`：切换手动/自动模式
- `FAN_ON`/`FAN_OFF`：手动开/关风扇
- `PUMP_ON`/`PUMP_OFF`：手动开/关水泵
- `LIGHT_ON`/`LIGHT_OFF`：手动开/关补光灯
- `RGB_HEART`/`RGB_SMILEY` 等：显示特定图案

## 依赖与环境

- 硬件平台：STM32F103ZET6
- 开发环境：Keil MDK 5.x
- 外设库：STM32F10x_StdPeriph_Driver
- 主要外设：DHT11、光敏电阻、风扇、水泵、LED、蜂鸣器、TFT LCD、WS2812 RGB、HC05蓝牙模块

## 参考文档

- `蓝牙通信实时测试指南.txt`：蓝牙配对与命令测试说明
- 各模块源码头文件，含详细注释

---

如需进一步定制或补充内容（如开发板原理图、详细接线图、功能演示视频等），请告知！ 