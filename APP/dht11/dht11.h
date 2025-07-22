#ifndef _DHT11_H_
#define _DHT11_H_

#include "system.h"
#include "SysTick.h"

// 添加bool类型支持（C89兼容）
#ifndef bool
#define bool u8
#define true 1
#define false 0
#endif

// DHT11引脚定义 - 修正为PG11匹配实际硬件连接
#define DHT11 (GPIO_Pin_11) //PG11
#define GPIO_DHT11 GPIOG

#define DHT11_DQ_IN PGin(11)	  // Input
#define DHT11_DQ_OUT PGout(11)  // Output

// DHT11增强功能常量定义
#define DHT11_FILTER_SIZE   5    // 滑动平均滤波器大小
#define DHT11_MAX_RETRY     3    // 最大重试次数

// DHT11状态枚举
typedef enum {
    DHT11_OK = 0,              // 读取成功
    DHT11_ERROR_TIMEOUT,       // 超时错误
    DHT11_ERROR_NO_RESPONSE,   // 传感器无响应
    DHT11_ERROR_INVALID_DATA   // 数据无效
} DHT11_Status_t;

// DHT11滤波器结构体
typedef struct {
    float temp_buffer[DHT11_FILTER_SIZE];  // 温度历史数据（修正字段名）
    float humi_buffer[DHT11_FILTER_SIZE];  // 湿度历史数据（修正字段名）
    u8 index;                              // 当前索引
    u8 count;                              // 数据计数（新增字段）
    bool is_full;                          // 缓冲区是否已满
} DHT11_Filter_t;

// DHT11数据结构体
typedef struct {
    u8 temperature;              // 当前温度
    u8 humidity;                 // 当前湿度
    u8 temp_filtered;            // 滤波后温度
    u8 humi_filtered;            // 滤波后湿度
    float temp_offset;           // 温度校准偏移
    float humi_offset;           // 湿度校准偏移
    bool calibration_enabled;    // 校准是否启用
    DHT11_Status_t status;       // 当前状态
    u16 error_count;             // 错误计数
    u8 retry_count;              // 重试计数（新增字段）
    u32 last_read_time;          // 上次读取时间
} DHT11_Data_t;

// DHT11 function declarations
void DHT11_IO_OUT(void);
void DHT11_IO_IN(void);
u8 DHT11_Init(void);
void DHT11_Rst(void);
u8 DHT11_Check(void);
u8 DHT11_Read_Bit(void);
u8 DHT11_Read_Byte(void);
u8 DHT11_Read_Data(u8 *temp,u8 *humi);

// 新增功能函数
u8 DHT11_Connection_Test(void);

// 增强功能函数声明
bool DHT11_Validate_Data(u8 temp, u8 humi);
void DHT11_Reset_Filter(DHT11_Filter_t* filter);
void DHT11_Set_Calibration(float temp_offset, float humi_offset);
DHT11_Data_t* DHT11_Get_Data_Handle(void);
DHT11_Status_t DHT11_Read_Data_Enhanced(DHT11_Data_t* data);
float DHT11_Filter_Data(float new_data, float* buffer, u8 size, u8* index, u8* count);
void DHT11_Update_With_Retry(void);

#endif
