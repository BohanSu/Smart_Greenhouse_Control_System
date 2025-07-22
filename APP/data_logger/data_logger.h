#ifndef __DATA_LOGGER_H__
#define __DATA_LOGGER_H__

#include "stm32f10x.h"
#include "../rtc/rtc.h"

// 数据记录类型
#define LOG_TYPE_SENSOR     0x01    // 传感器数据
#define LOG_TYPE_OPERATION  0x02    // 操作记录
#define LOG_TYPE_ALARM      0x03    // 报警记录
#define LOG_TYPE_SYSTEM     0x04    // 系统事件

// 操作类型
#define OP_FAN_ON           0x01
#define OP_FAN_OFF          0x02
#define OP_PUMP_ON          0x03
#define OP_PUMP_OFF         0x04
#define OP_LIGHT_ON         0x05
#define OP_LIGHT_OFF        0x06
#define OP_MODE_AUTO        0x07
#define OP_MODE_MANUAL      0x08

// 报警类型
#define ALARM_HIGH_TEMP_LOG     0x01
#define ALARM_LOW_TEMP_LOG      0x02
#define ALARM_HIGH_HUMI_LOG     0x03
#define ALARM_LOW_HUMI_LOG      0x04
#define ALARM_LOW_LIGHT_LOG     0x05
#define ALARM_SENSOR_ERROR_LOG  0x06

// 存储配置
#define FLASH_START_ADDR    0x08070000  // 从448KB开始使用64KB作为数据存储
#define FLASH_END_ADDR      0x0807FFFF
#define FLASH_PAGE_SIZE     2048        // 页大小2KB
#define FLASH_SECTOR_SIZE   4096        // 扇区大小4KB

#define LOG_RECORD_SIZE     16          // 每条记录16字节
#define LOG_RECORDS_PER_PAGE (FLASH_PAGE_SIZE / LOG_RECORD_SIZE)  // 每页记录数
#define MAX_LOG_RECORDS     ((FLASH_END_ADDR - FLASH_START_ADDR + 1) / LOG_RECORD_SIZE)

// 传感器数据记录结构
typedef struct
{
    u32 timestamp;      // 时间戳
    u8  log_type;       // 记录类型
    u8  temperature;    // 温度
    u8  humidity;       // 湿度
    u8  light;          // 光照
    u8  fan_status;     // 风扇状态
    u8  pump_status;    // 水泵状态
    u8  light_status;   // 补光灯状态
    u8  work_mode;      // 工作模式
    u8  alarm_flags;    // 报警标志
    u8  reserved[3];    // 保留字段
} __attribute__((packed)) SensorLogRecord_t;

// 操作记录结构
typedef struct
{
    u32 timestamp;      // 时间戳
    u8  log_type;       // 记录类型
    u8  operation;      // 操作类型
    u8  old_value;      // 操作前值
    u8  new_value;      // 操作后值
    u8  trigger_mode;   // 触发模式（0-自动，1-手动，2-定时）
    u8  reserved[7];    // 保留字段
} __attribute__((packed)) OperationLogRecord_t;

// 报警记录结构
typedef struct
{
    u32 timestamp;      // 时间戳
    u8  log_type;       // 记录类型
    u8  alarm_type;     // 报警类型
    u8  alarm_level;    // 报警级别（1-低，2-中，3-高）
    u8  trigger_value;  // 触发值
    u8  threshold;      // 阈值
    u8  duration;       // 持续时间（分钟）
    u8  reserved[6];    // 保留字段
} __attribute__((packed)) AlarmLogRecord_t;

// 通用记录结构
typedef union
{
    u8 raw_data[16];
    SensorLogRecord_t sensor;
    OperationLogRecord_t operation;
    AlarmLogRecord_t alarm;
} LogRecord_t;

// 数据统计结构
typedef struct
{
    u32 total_records;      // 总记录数
    u32 sensor_records;     // 传感器记录数
    u32 operation_records;  // 操作记录数
    u32 alarm_records;      // 报警记录数
    u32 oldest_timestamp;   // 最旧记录时间戳
    u32 newest_timestamp;   // 最新记录时间戳
    u16 current_page;       // 当前写入页
    u16 current_offset;     // 当前页内偏移
} DataLoggerInfo_t;

// 查询条件结构
typedef struct
{
    u32 start_time;     // 开始时间
    u32 end_time;       // 结束时间
    u8  log_type;       // 记录类型（0-全部）
    u16 max_records;    // 最大返回记录数
} LogQuery_t;

// 函数声明
u8 DataLogger_Init(void);                              // 初始化数据记录器
u8 DataLogger_WriteSensorData(u8 temp, u8 humi, u8 light, 
                              u8 fan, u8 pump, u8 light_dev, 
                              u8 mode, u8 alarms);     // 记录传感器数据
u8 DataLogger_WriteOperation(u8 operation, u8 old_val, 
                            u8 new_val, u8 trigger);   // 记录操作
u8 DataLogger_WriteAlarm(u8 alarm_type, u8 level, 
                        u8 trigger_val, u8 threshold); // 记录报警
u16 DataLogger_Query(LogQuery_t* query, LogRecord_t* records, u16 buffer_size); // 查询记录
u8 DataLogger_GetInfo(DataLoggerInfo_t* info);         // 获取统计信息
u8 DataLogger_EraseAll(void);                          // 清空所有记录
u8 DataLogger_GetDailyStats(u32 date, u8* avg_temp, u8* avg_humi, 
                           u8* avg_light, u16* operations); // 获取日统计
void DataLogger_PrintRecords(u16 count);               // 打印最近记录

// 内部函数
static u8 DataLogger_FindNextPage(void);               // 查找下一可用页
static u8 DataLogger_WriteRecord(LogRecord_t* record); // 写入记录
static u8 DataLogger_ReadRecord(u32 addr, LogRecord_t* record); // 读取记录
static u8 DataLogger_ErasePage(u16 page);              // 擦除页
static u32 DataLogger_GetTimestamp(void);              // 获取时间戳

// 全局变量
extern DataLoggerInfo_t logger_info;

#endif /* __DATA_LOGGER_H__ */
 
