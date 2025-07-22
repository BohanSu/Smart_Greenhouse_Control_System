#include "config.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

// 全局变量
SystemConfig_t system_config;

// 参数信息表
const ConfigItemInfo_t config_items[CONFIG_ITEM_COUNT] = {
    {"temp_fan_on",     10, 50, DEFAULT_TEMP_FAN_ON,    "°C", "风扇开启温度"},
    {"temp_high_alarm", 25, 60, DEFAULT_TEMP_HIGH_ALARM, "°C", "高温报警阈值"},
    {"temp_low_alarm",  0,  25, DEFAULT_TEMP_LOW_ALARM,  "°C", "低温报警阈值"},
    {"humi_pump_on",    10, 80, DEFAULT_HUMI_PUMP_ON,    "%",  "水泵开启湿度"},
    {"humi_high_alarm", 60,100, DEFAULT_HUMI_HIGH_ALARM, "%",  "高湿度报警"},
    {"humi_low_alarm",  0,  40, DEFAULT_HUMI_LOW_ALARM,  "%",  "低湿度报警"},
    {"light_auto_on",   10, 90, DEFAULT_LIGHT_AUTO_ON,   "%",  "补光灯开启光照"},
    {"light_low_alarm", 0,  50, DEFAULT_LIGHT_LOW_ALARM, "%",  "光照不足报警"},
    {"sensor_interval", 1,  60, DEFAULT_SENSOR_INTERVAL, "s",  "传感器读取间隔"},
    {"log_interval",    5, 300, DEFAULT_LOG_INTERVAL,    "s",  "数据记录间隔"},
    {"auto_mode",       0,   1, 0,                       "",   "默认工作模式"},
    {"alarm_sound",     0,   1, 1,                       "",   "报警声音使能"},
    {"led_brightness",  0, 255, 255,                     "",   "LED亮度"},
    {"morning_start",   0,  23, DEFAULT_MORNING_START,   "h",  "白天开始时间"},
    {"night_start",     0,  23, DEFAULT_NIGHT_START,     "h",  "夜晚开始时间"},
    {"auto_light_time", 0,1440, DEFAULT_AUTO_LIGHT_TIME, "min","自动补光时间"}
};

// Flash操作函数
static void Config_Flash_Unlock(void)
{
    FLASH_Unlock();
}

static void Config_Flash_Lock(void)
{
    FLASH_Lock();
}

// 计算校验和
u32 Config_Calculate_Checksum(SystemConfig_t* config)
{
    u32 checksum = 0;
    u8* data = (u8*)config;
    u16 len = sizeof(SystemConfig_t) - sizeof(u32); // 不包括校验和字段
    
    for(u16 i = 0; i < len; i++)
    {
        checksum += data[i];
    }
    
    return checksum;
}

// 设置默认配置
static void Config_Set_Defaults(void)
{
    system_config.magic = CONFIG_MAGIC;
    system_config.version = CONFIG_VERSION;
    
    // 温度参数
    system_config.temp_fan_on = DEFAULT_TEMP_FAN_ON;
    system_config.temp_high_alarm = DEFAULT_TEMP_HIGH_ALARM;
    system_config.temp_low_alarm = DEFAULT_TEMP_LOW_ALARM;
    system_config.temp_hysteresis = DEFAULT_AUTO_HYSTERESIS;
    
    // 湿度参数
    system_config.humi_pump_on = DEFAULT_HUMI_PUMP_ON;
    system_config.humi_high_alarm = DEFAULT_HUMI_HIGH_ALARM;
    system_config.humi_low_alarm = DEFAULT_HUMI_LOW_ALARM;
    system_config.humi_hysteresis = 5; // 5%滞回
    
    // 光照参数
    system_config.light_auto_on = DEFAULT_LIGHT_AUTO_ON;
    system_config.light_low_alarm = DEFAULT_LIGHT_LOW_ALARM;
    system_config.light_hysteresis = 10; // 10%滞回
    
    // 时间参数
    system_config.morning_start = DEFAULT_MORNING_START;
    system_config.night_start = DEFAULT_NIGHT_START;
    system_config.auto_light_time = DEFAULT_AUTO_LIGHT_TIME;
    
    // 系统参数
    system_config.sensor_interval = DEFAULT_SENSOR_INTERVAL;
    system_config.log_interval = DEFAULT_LOG_INTERVAL;
    system_config.auto_mode_default = 0; // 默认自动模式
    
    // 高级参数
    system_config.alarm_sound_enable = 1; // 启用报警声音
    system_config.led_brightness = 255;   // 最大亮度
    system_config.auto_shutdown_enable = 0; // 禁用自动关机
    
    // 清空保留字段
    memset(system_config.reserved, 0, sizeof(system_config.reserved));
    
    // 计算校验和
    system_config.checksum = Config_Calculate_Checksum(&system_config);
}

// 初始化配置系统
u8 Config_Init(void)
{
    printf("初始化配置系统...\r\n");
    
    // 尝试从Flash加载配置
    if(Config_Load() != 0)
    {
        printf("配置加载失败，使用默认配置\r\n");
        Config_Set_Defaults();
        Config_Save();
    }
    
    printf("配置系统初始化完成\r\n");
    return 0;
}

// 从Flash加载配置
u8 Config_Load(void)
{
    SystemConfig_t* flash_config = (SystemConfig_t*)CONFIG_FLASH_ADDR;
    
    // 检查魔术字
    if(flash_config->magic != CONFIG_MAGIC)
    {
        printf("配置魔术字不匹配: 0x%08X\r\n", flash_config->magic);
        return 1;
    }
    
    // 检查版本
    if(flash_config->version != CONFIG_VERSION)
    {
        printf("配置版本不匹配: %d\r\n", flash_config->version);
        return 1;
    }
    
    // 复制配置到RAM
    memcpy(&system_config, flash_config, sizeof(SystemConfig_t));
    
    // 验证校验和
    u32 calculated_checksum = Config_Calculate_Checksum(&system_config);
    if(calculated_checksum != system_config.checksum)
    {
        printf("配置校验和错误: 计算=%08X, 存储=%08X\r\n", 
               calculated_checksum, system_config.checksum);
        return 1;
    }
    
    printf("配置加载成功\r\n");
    return 0;
}

// 保存配置到Flash
u8 Config_Save(void)
{
    FLASH_Status status;
    u32* data_ptr = (u32*)&system_config;
    u32 addr = CONFIG_FLASH_ADDR;
    
    printf("保存配置到Flash...\r\n");
    
    // 更新校验和
    system_config.checksum = Config_Calculate_Checksum(&system_config);
    
    // 解锁Flash
    Config_Flash_Unlock();
    
    // 擦除页
    status = FLASH_ErasePage(CONFIG_FLASH_ADDR);
    if(status != FLASH_COMPLETE)
    {
        printf("Flash页擦除失败: %d\r\n", status);
        Config_Flash_Lock();
        return 1;
    }
    
    // 写入数据
    for(u16 i = 0; i < sizeof(SystemConfig_t) / 4; i++)
    {
        status = FLASH_ProgramWord(addr + i * 4, data_ptr[i]);
        if(status != FLASH_COMPLETE)
        {
            printf("Flash写入失败: 地址0x%08X, 状态%d\r\n", addr + i * 4, status);
            Config_Flash_Lock();
            return 1;
        }
    }
    
    // 锁定Flash
    Config_Flash_Lock();
    
    printf("配置保存成功\r\n");
    return 0;
}

// 恢复默认配置
u8 Config_Reset(void)
{
    printf("恢复默认配置...\r\n");
    Config_Set_Defaults();
    return Config_Save();
}

// 验证配置有效性
u8 Config_Validate(void)
{
    u8 errors = 0;
    
    // 检查温度参数
    if(system_config.temp_low_alarm >= system_config.temp_high_alarm)
    {
        printf("温度报警参数错误: 低温%d >= 高温%d\r\n", 
               system_config.temp_low_alarm, system_config.temp_high_alarm);
        errors++;
    }
    
    // 检查湿度参数
    if(system_config.humi_low_alarm >= system_config.humi_high_alarm)
    {
        printf("湿度报警参数错误: 低湿度%d >= 高湿度%d\r\n", 
               system_config.humi_low_alarm, system_config.humi_high_alarm);
        errors++;
    }
    
    // 检查时间参数
    if(system_config.morning_start >= system_config.night_start)
    {
        printf("时间参数错误: 白天开始%d >= 夜晚开始%d\r\n", 
               system_config.morning_start, system_config.night_start);
        errors++;
    }
    
    return errors;
}

// 获取8位参数
u8 Config_Get_U8(ConfigItem_t item)
{
    switch(item)
    {
        case CONFIG_TEMP_FAN_ON:        return system_config.temp_fan_on;
        case CONFIG_TEMP_HIGH_ALARM:    return system_config.temp_high_alarm;
        case CONFIG_TEMP_LOW_ALARM:     return system_config.temp_low_alarm;
        case CONFIG_HUMI_PUMP_ON:       return system_config.humi_pump_on;
        case CONFIG_HUMI_HIGH_ALARM:    return system_config.humi_high_alarm;
        case CONFIG_HUMI_LOW_ALARM:     return system_config.humi_low_alarm;
        case CONFIG_LIGHT_AUTO_ON:      return system_config.light_auto_on;
        case CONFIG_LIGHT_LOW_ALARM:    return system_config.light_low_alarm;
        case CONFIG_SENSOR_INTERVAL:    return system_config.sensor_interval;
        case CONFIG_LOG_INTERVAL:       return system_config.log_interval;
        case CONFIG_AUTO_MODE_DEFAULT:  return system_config.auto_mode_default;
        case CONFIG_ALARM_SOUND_ENABLE: return system_config.alarm_sound_enable;
        case CONFIG_LED_BRIGHTNESS:     return system_config.led_brightness;
        case CONFIG_MORNING_START:      return system_config.morning_start;
        case CONFIG_NIGHT_START:        return system_config.night_start;
        default: return 0;
    }
}

// 获取16位参数
u16 Config_Get_U16(ConfigItem_t item)
{
    switch(item)
    {
        case CONFIG_AUTO_LIGHT_TIME:    return system_config.auto_light_time;
        default: return Config_Get_U8(item);
    }
}

// 设置8位参数
u8 Config_Set_U8(ConfigItem_t item, u8 value)
{
    const ConfigItemInfo_t* info = &config_items[item];
    
    // 检查范围
    if(value < info->min_value || value > info->max_value)
    {
        printf("参数值超出范围: %s = %d (范围: %d-%d)\r\n", 
               info->name, value, info->min_value, info->max_value);
        return 1;
    }
    
    switch(item)
    {
        case CONFIG_TEMP_FAN_ON:        system_config.temp_fan_on = value; break;
        case CONFIG_TEMP_HIGH_ALARM:    system_config.temp_high_alarm = value; break;
        case CONFIG_TEMP_LOW_ALARM:     system_config.temp_low_alarm = value; break;
        case CONFIG_HUMI_PUMP_ON:       system_config.humi_pump_on = value; break;
        case CONFIG_HUMI_HIGH_ALARM:    system_config.humi_high_alarm = value; break;
        case CONFIG_HUMI_LOW_ALARM:     system_config.humi_low_alarm = value; break;
        case CONFIG_LIGHT_AUTO_ON:      system_config.light_auto_on = value; break;
        case CONFIG_LIGHT_LOW_ALARM:    system_config.light_low_alarm = value; break;
        case CONFIG_SENSOR_INTERVAL:    system_config.sensor_interval = value; break;
        case CONFIG_LOG_INTERVAL:       system_config.log_interval = value; break;
        case CONFIG_AUTO_MODE_DEFAULT:  system_config.auto_mode_default = value; break;
        case CONFIG_ALARM_SOUND_ENABLE: system_config.alarm_sound_enable = value; break;
        case CONFIG_LED_BRIGHTNESS:     system_config.led_brightness = value; break;
        case CONFIG_MORNING_START:      system_config.morning_start = value; break;
        case CONFIG_NIGHT_START:        system_config.night_start = value; break;
        default: return 1;
    }
    
    printf("设置参数: %s = %d%s\r\n", info->name, value, info->unit);
    return 0;
}

// 设置16位参数
u8 Config_Set_U16(ConfigItem_t item, u16 value)
{
    switch(item)
    {
        case CONFIG_AUTO_LIGHT_TIME:
            if(value > 1440) // 最大24小时
            {
                printf("自动补光时间超出范围: %d分钟\r\n", value);
                return 1;
            }
            system_config.auto_light_time = value;
            printf("设置参数: auto_light_time = %d分钟\r\n", value);
            return 0;
        default:
            return Config_Set_U8(item, (u8)value);
    }
}

// 获取所有配置
u8 Config_Get_All(SystemConfig_t* config)
{
    *config = system_config;
    return 0;
}

// 设置所有配置
u8 Config_Set_All(SystemConfig_t* config)
{
    system_config = *config;
    return Config_Validate();
}

// 打印所有配置
void Config_Print_All(void)
{
    printf("=== 系统配置参数 ===\r\n");
    
    for(u8 i = 0; i < CONFIG_ITEM_COUNT; i++)
    {
        Config_Print_Item((ConfigItem_t)i);
    }
    
    printf("==================\r\n");
}

// 打印单个配置项
void Config_Print_Item(ConfigItem_t item)
{
    const ConfigItemInfo_t* info = &config_items[item];
    
    if(item == CONFIG_AUTO_LIGHT_TIME)
    {
        printf("%-15s: %4d%s (%s)\r\n", 
               info->name, Config_Get_U16(item), info->unit, info->description);
    }
    else
    {
        printf("%-15s: %4d%s (%s)\r\n", 
               info->name, Config_Get_U8(item), info->unit, info->description);
    }
}

// 获取参数信息
const ConfigItemInfo_t* Config_Get_ItemInfo(ConfigItem_t item)
{
    if(item < CONFIG_ITEM_COUNT)
        return &config_items[item];
    return NULL;
}

// 处理配置命令
void Config_Handle_Command(char* cmd)
{
    char* token;
    char* param_name;
    char* param_value;
    int value;
    
    // 解析命令
    token = strtok(cmd, " ");
    if(token == NULL) return;
    
    if(strstr(token, "CONFIG_GET"))
    {
        param_name = strtok(NULL, " ");
        if(param_name == NULL)
        {
            // 显示所有参数
            Config_Print_All();
            return;
        }
        
        // 查找参数
        for(u8 i = 0; i < CONFIG_ITEM_COUNT; i++)
        {
            if(strstr(config_items[i].name, param_name))
            {
                Config_Print_Item((ConfigItem_t)i);
                return;
            }
        }
        printf("未找到参数: %s\r\n", param_name);
    }
    else if(strstr(token, "CONFIG_SET"))
    {
        param_name = strtok(NULL, " ");
        param_value = strtok(NULL, " ");
        
        if(param_name == NULL || param_value == NULL)
        {
            printf("用法: CONFIG_SET <参数名> <值>\r\n");
            return;
        }
        
        value = atoi(param_value);
        
        // 查找并设置参数
        for(u8 i = 0; i < CONFIG_ITEM_COUNT; i++)
        {
            if(strstr(config_items[i].name, param_name))
            {
                if(i == CONFIG_AUTO_LIGHT_TIME)
                {
                    if(Config_Set_U16((ConfigItem_t)i, value) == 0)
                        printf("参数设置成功\r\n");
                }
                else
                {
                    if(Config_Set_U8((ConfigItem_t)i, value) == 0)
                        printf("参数设置成功\r\n");
                }
                return;
            }
        }
        printf("未找到参数: %s\r\n", param_name);
    }
    else if(strstr(token, "CONFIG_SAVE"))
    {
        if(Config_Save() == 0)
            printf("配置保存成功\r\n");
        else
            printf("配置保存失败\r\n");
    }
    else if(strstr(token, "CONFIG_RESET"))
    {
        if(Config_Reset() == 0)
            printf("配置重置成功\r\n");
        else
            printf("配置重置失败\r\n");
    }
    else if(strstr(token, "CONFIG_HELP"))
    {
        printf("配置命令帮助:\r\n");
        printf("CONFIG_GET [参数名] - 查看配置\r\n");
        printf("CONFIG_SET <参数名> <值> - 设置配置\r\n");
        printf("CONFIG_SAVE - 保存配置\r\n");
        printf("CONFIG_RESET - 恢复默认\r\n");
        printf("CONFIG_HELP - 显示帮助\r\n");
    }
} 