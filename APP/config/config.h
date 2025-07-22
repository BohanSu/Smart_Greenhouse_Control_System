#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "stm32f10x.h"

// 配置参数版本号
#define CONFIG_VERSION      0x01

// 配置存储地址（使用BKP寄存器和Flash）
#define CONFIG_FLASH_ADDR   0x0806F000  // 使用Flash最后4KB存储配置
#define CONFIG_MAGIC        0x5A5A5A5A  // 配置有效标志

// 默认配置参数
#define DEFAULT_TEMP_FAN_ON         30  // 温度超过30°C开启风扇
#define DEFAULT_TEMP_HIGH_ALARM     35  // 高温报警
#define DEFAULT_TEMP_LOW_ALARM      15  // 低温报警

#define DEFAULT_HUMI_PUMP_ON        30  // 湿度低于30%开启水泵
#define DEFAULT_HUMI_HIGH_ALARM     80  // 高湿度报警
#define DEFAULT_HUMI_LOW_ALARM      20  // 低湿度报警

#define DEFAULT_LIGHT_AUTO_ON       30  // 光照低于30%开启补光灯
#define DEFAULT_LIGHT_LOW_ALARM     20  // 光照不足报警

#define DEFAULT_SENSOR_INTERVAL     2   // 传感器读取间隔（秒）
#define DEFAULT_LOG_INTERVAL        10  // 数据记录间隔（秒）
#define DEFAULT_AUTO_HYSTERESIS     2   // 自动控制滞回值

// 时间相关默认值
#define DEFAULT_MORNING_START       6   // 早晨开始时间
#define DEFAULT_NIGHT_START         22  // 夜晚开始时间
#define DEFAULT_AUTO_LIGHT_TIME     480 // 自动补光时间（分钟）

// 系统配置结构体
typedef struct
{
    u32 magic;              // 配置有效标志
    u8  version;            // 配置版本号
    
    // 温度控制参数
    u8  temp_fan_on;        // 风扇开启温度阈值
    u8  temp_high_alarm;    // 高温报警阈值
    u8  temp_low_alarm;     // 低温报警阈值
    u8  temp_hysteresis;    // 温度控制滞回
    
    // 湿度控制参数
    u8  humi_pump_on;       // 水泵开启湿度阈值
    u8  humi_high_alarm;    // 高湿度报警阈值
    u8  humi_low_alarm;     // 低湿度报警阈值
    u8  humi_hysteresis;    // 湿度控制滞回
    
    // 光照控制参数
    u8  light_auto_on;      // 补光灯开启光照阈值
    u8  light_low_alarm;    // 光照不足报警阈值
    u8  light_hysteresis;   // 光照控制滞回
    
    // 时间控制参数
    u8  morning_start;      // 白天开始时间
    u8  night_start;        // 夜晚开始时间
    u16 auto_light_time;    // 自动补光时间（分钟）
    
    // 系统参数
    u8  sensor_interval;    // 传感器读取间隔（秒）
    u8  log_interval;       // 数据记录间隔（秒）
    u8  auto_mode_default;  // 启动时默认模式（0-自动，1-手动）
    
    // 高级参数
    u8  alarm_sound_enable; // 报警声音使能
    u8  led_brightness;     // LED亮度（0-255）
    u8  auto_shutdown_enable; // 自动关机使能（夜间节能）
    
    // 备用参数
    u8  reserved[16];       // 保留字段
    
    u32 checksum;           // 校验和
} __attribute__((packed)) SystemConfig_t;

// 参数项枚举
typedef enum
{
    CONFIG_TEMP_FAN_ON = 0,
    CONFIG_TEMP_HIGH_ALARM,
    CONFIG_TEMP_LOW_ALARM,
    CONFIG_HUMI_PUMP_ON,
    CONFIG_HUMI_HIGH_ALARM,
    CONFIG_HUMI_LOW_ALARM,
    CONFIG_LIGHT_AUTO_ON,
    CONFIG_LIGHT_LOW_ALARM,
    CONFIG_SENSOR_INTERVAL,
    CONFIG_LOG_INTERVAL,
    CONFIG_AUTO_MODE_DEFAULT,
    CONFIG_ALARM_SOUND_ENABLE,
    CONFIG_LED_BRIGHTNESS,
    CONFIG_MORNING_START,
    CONFIG_NIGHT_START,
    CONFIG_AUTO_LIGHT_TIME,
    CONFIG_ITEM_COUNT
} ConfigItem_t;

// 参数信息结构
typedef struct
{
    const char* name;       // 参数名称
    u8 min_value;           // 最小值
    u8 max_value;           // 最大值
    u8 default_value;       // 默认值
    const char* unit;       // 单位
    const char* description; // 描述
} ConfigItemInfo_t;

// 函数声明
u8 Config_Init(void);                           // 配置系统初始化
u8 Config_Load(void);                           // 加载配置
u8 Config_Save(void);                           // 保存配置
u8 Config_Reset(void);                          // 恢复默认配置
u8 Config_Validate(void);                       // 验证配置有效性

// 参数访问函数
u8 Config_Get_U8(ConfigItem_t item);            // 获取8位参数
u16 Config_Get_U16(ConfigItem_t item);          // 获取16位参数
u8 Config_Set_U8(ConfigItem_t item, u8 value); // 设置8位参数
u8 Config_Set_U16(ConfigItem_t item, u16 value); // 设置16位参数

// 批量操作
u8 Config_Get_All(SystemConfig_t* config);     // 获取所有配置
u8 Config_Set_All(SystemConfig_t* config);     // 设置所有配置

// 工具函数
u32 Config_Calculate_Checksum(SystemConfig_t* config); // 计算校验和
void Config_Print_All(void);                   // 打印所有配置
void Config_Print_Item(ConfigItem_t item);     // 打印单个配置项
const ConfigItemInfo_t* Config_Get_ItemInfo(ConfigItem_t item); // 获取参数信息

// 蓝牙配置命令处理
void Config_Handle_Command(char* cmd);          // 处理配置命令

// 全局变量
extern SystemConfig_t system_config;
extern const ConfigItemInfo_t config_items[];

#endif /* __CONFIG_H__ */
 
