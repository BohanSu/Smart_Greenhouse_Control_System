#ifndef _greenhouse_control_H
#define _greenhouse_control_H

#include "system.h"
#include "dht11.h"
#include "lsens.h"
#include "led.h"
#include "beep.h"
#include "key.h"
#include "../rtc/rtc.h"
#include "../data_logger/data_logger.h"
#include "../config/config.h"

// 添加bool类型定义（针对C89标准）
#ifndef bool
#define bool unsigned char
#define true 1
#define false 0
#endif

// 系统工作模式
#define MODE_AUTO       0   // 自动模式
#define MODE_MANUAL     1   // 手动模式

// 设备状态定义
#define DEVICE_OFF      0
#define DEVICE_ON       1

// 报警类型定义
#define ALARM_NONE          0x00
#define ALARM_HIGH_TEMP     0x01
#define ALARM_LOW_TEMP      0x02
#define ALARM_HIGH_HUMI     0x04
#define ALARM_LOW_HUMI      0x08
#define ALARM_LOW_LIGHT     0x10
#define ALARM_SENSOR_ERROR  0x20

// 阈值设定
#define TEMP_AUTO_FAN_ON    27  // 温度超过27°C开启风扇
#define TEMP_HIGH_ALARM     35  // 高温报警
#define TEMP_LOW_ALARM      15  // 低温报警

#define HUMI_AUTO_PUMP_ON   30  // 湿度低于30%开启水泵
#define HUMI_HIGH_ALARM     80  // 高湿度报警
#define HUMI_LOW_ALARM      20  // 低湿度报警

#define LIGHT_AUTO_ON       30  // 光照低于30%开启补光灯
#define LIGHT_LOW_ALARM     20  // 光照不足报警

// 智能控制参数 - 🚀 减少滞回控制差值，提高精度和响应速度
#define HUMI_HYSTERESIS     5   // 湿度滞回控制差值
#define LIGHT_HYSTERESIS    10  // 光照滞回控制差值
#define TEMP_HYSTERESIS     2   // 温度滞回控制差值，整数值

// 设备运行时间控制 - 🚀 大幅减少响应时间，实现即时控制
#define FAN_MIN_RUN_TIME    10000 // 风扇最短运行10秒
#define PUMP_MIN_RUN_TIME   5000  // 水泵最短运行5秒
#define LIGHT_MIN_RUN_TIME  1000    // 补光灯最小运行时间1秒（原5秒）

// 控制算法类型
typedef enum {
    CONTROL_SIMPLE = 0,     // 简单开关控制
    CONTROL_HYSTERESIS,     // 滞回控制
    CONTROL_PID             // PID控制(预留)
} ControlMode_t;

// 设备运行状态
typedef struct {
    u8 status;              // 当前状态
    u32 last_on_time;       // 上次开启时间
    u32 last_off_time;      // 上次关闭时间
    u32 total_run_time;     // 总运行时间
    u16 switch_count;       // 开关次数
} DeviceRunStatus_t;

// 环境数据历史记录
typedef struct {
    float temp_history[10]; // 温度历史(最近10次)
    float humi_history[10]; // 湿度历史(最近10次)
    u8 history_index;       // 历史数据索引
    u8 history_count;       // 历史数据数量
} EnvironmentHistory_t;

// 系统状态结构体
typedef struct
{
    u8 temperature;     // 当前温度
    u8 humidity;        // 当前湿度
    u8 light;          // 当前光照强度
    
    u8 work_mode;      // 工作模式
    u8 fan_status;     // 风扇状态
    u8 pump_status;    // 水泵状态
    u8 light_status;   // 补光灯状态
    
    u8 alarm_flags;    // 报警标志
    u8 sensor_error;   // 传感器错误标志
    
    // 增强功能字段
    ControlMode_t control_mode;         // 控制算法模式
    DeviceRunStatus_t fan_run_status;   // 风扇运行状态
    DeviceRunStatus_t pump_run_status;  // 水泵运行状态
    DeviceRunStatus_t light_run_status; // 补光灯运行状态
    EnvironmentHistory_t env_history;   // 环境数据历史
    
    // 系统运行统计
    u32 system_run_time;    // 系统运行时间
    u16 alarm_count;        // 报警次数
    u8 auto_mode_ratio;     // 自动模式使用率
    
} GreenhouseStatus_t;

extern GreenhouseStatus_t greenhouse_status;

// 功能函数声明
void Greenhouse_Init(void);                     // 温室系统初始化
void Greenhouse_Task(void);                     // 温室控制主任务
void Greenhouse_Read_Sensors(void);             // 读取传感器数据
void Greenhouse_Auto_Control(void);             // 自动控制算法
void Greenhouse_Manual_Control(u8 device, u8 action); // 手动控制
void Greenhouse_Process_Keys(void);             // 处理按键输入
void Greenhouse_Check_Alarms(void);             // 检查报警条件
void Greenhouse_Update_Display(void);           // 更新显示状态
GreenhouseStatus_t* Greenhouse_Get_Status(void); // 获取系统状态
void Greenhouse_Update_Sensors(void);
void Greenhouse_Handle_Bluetooth(void);
void Greenhouse_Display_Status(void);
void Greenhouse_Process_Key(u8 key);

// 控制函数
void Greenhouse_Fan_Control(u8 state);          // 风扇控制
void Greenhouse_Pump_Control(u8 state);         // 水泵控制
void Greenhouse_Light_Control(u8 state);        // 补光灯控制
void Greenhouse_Set_Mode(u8 mode);      // 设置工作模式

// 智能控制增强函数
void Greenhouse_Smart_Control(void);            // 智能控制算法
void Greenhouse_Hysteresis_Control(void);       // 滞回控制算法
void Greenhouse_Update_History(void);           // 更新环境数据历史
float Greenhouse_Get_Trend(float* history, u8 count); // 获取数据趋势
bool Greenhouse_Device_Can_Switch(DeviceRunStatus_t* device, u32 min_time); // 检查设备是否可以切换
void Greenhouse_Update_Device_Status(DeviceRunStatus_t* device, u8 new_status); // 更新设备状态
void Greenhouse_Predictive_Control(void);       // 预测性控制
void Greenhouse_Energy_Optimize(void);          // 能耗优化
void Greenhouse_Safety_Check(void);             // 安全检查

#endif
 
