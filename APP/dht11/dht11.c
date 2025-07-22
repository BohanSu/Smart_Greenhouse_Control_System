#include "dht11.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stm32f10x.h"  // 添加STM32头文件，解决GPIO定义问题

// 外部全局变量声明
extern volatile u32 system_time_ms;

// 增强功能全局变量
static DHT11_Data_t dht11_data = {0};
static DHT11_Filter_t dht11_filter = {0};

// Set DHT11 IO to output mode
void DHT11_IO_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 确保GPIOG时钟已启用
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = DHT11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_DHT11, &GPIO_InitStructure);
}

// Set DHT11 IO to input mode
void DHT11_IO_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 确保GPIOG时钟已启用
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = DHT11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_Init(GPIO_DHT11, &GPIO_InitStructure);
}

// Reset DHT11
void DHT11_Rst(void)	   
{                 
    DHT11_IO_OUT(); // Set to output mode
    DHT11_DQ_OUT = 0; // Pull data line down
    delay_ms(20); // At least 18ms low pulse (DHT11 datasheet requirement)
    DHT11_DQ_OUT = 1; // Pull data line up
    delay_us(30); // Wait 20-40us before reading (DHT11 datasheet requirement)
}

// Check DHT11 existence
// Return 1: not exist
// Return 0: exist
u8 DHT11_Check(void)	   
{   
    u8 retry = 0;
    
    DHT11_IO_IN(); // Set to input mode
    
    while (DHT11_DQ_IN && retry < 100) // DHT11 will pull down data pin 40~80us
    {
        retry++;
        delay_us(1);
    }
    
    if(retry >= 100) {
        return 1;
    }
    
    retry = 0;
    while (!DHT11_DQ_IN && retry < 100) // DHT11 will pull up data pin 40~80us
    {
        retry++;
        delay_us(1);
    }
    
    if(retry >= 100) {
        return 1;
    }
    
    return 0;
}

// Read one bit from DHT11
// Return value: 1/0
u8 DHT11_Read_Bit(void)
{
    u8 retry = 0;
    
    while(DHT11_DQ_IN && retry < 100) //Wait for low level
    {
        retry++;
        delay_us(1);
    }
    retry = 0;
    
    while(!DHT11_DQ_IN && retry < 100) //Wait for high level
    {
        retry++;
        delay_us(1);
    }
    delay_us(40); //Wait 40us
    
    if(DHT11_DQ_IN) return 1;
    else return 0;
}

// Read one byte from DHT11
// Return value: read data
u8 DHT11_Read_Byte(void)
{
    u8 i, dat;
    dat = 0;
    
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    
    return dat;
}

// Initialize DHT11 IO port DQ and check DHT11 existence
// Return 1: not exist
// Return 0: exist
u8 DHT11_Init(void)
{
    u8 result;
    
    // 初始化GPIO
    DHT11_IO_OUT();
    DHT11_DQ_OUT = 1;
    delay_ms(100);  // 等待DHT11上电稳定
    
    // 测试传感器连接
    DHT11_Rst();
    result = DHT11_Check();
    
    if(result == 0) {
        printf("DHT11: Sensor Init Success (PG11)\r\n");
        return 0;  // 成功
    } else {
        printf("DHT11: Sensor Connection Failed (PG11)\r\n");
        return 1;  // 失败
    }
}

// Read data from DHT11
// temp: temperature value (range: 0~50°C)
// humi: humidity value (range: 20%~90%)
// Return value: 0-normal, 1-read failed
u8 DHT11_Read_Data(u8 *temp, u8 *humi)
{
    u8 buf[5];
    u8 i;
    
    DHT11_Rst();
    if(DHT11_Check() == 0)
    {
        for(i = 0; i < 5; i++) //读取40位数据
        {
            buf[i] = DHT11_Read_Byte();
        }
        if((buf[0]+buf[1]+buf[2]+buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
    } else {
        return 1;
    }
    return 0;
}

// DHT11连接测试
u8 DHT11_Connection_Test(void)
{
    u8 input_state;
    int i;
    
    printf("DHT11: Starting connection test...\r\n");
    
    // 测试1: GPIO基本功能测试
    printf("Test 1: GPIO Basic Function\r\n");
    DHT11_IO_OUT();
    DHT11_DQ_OUT = 0;
    delay_ms(1);
    if(GPIO_ReadOutputDataBit(GPIOG, DHT11) == 0) {
        printf("  - Output LOW: OK\r\n");
    } else {
        printf("  - Output LOW: FAILED\r\n");
        return 1;
    }
    
    DHT11_DQ_OUT = 1;
    delay_ms(1);
    if(GPIO_ReadOutputDataBit(GPIOG, DHT11) == 1) {
        printf("  - 输出高电平: OK\r\n");
    } else {
        printf("  - 输出高电平: FAILED\r\n");
        return 1;
    }
    
    // 测试2: 输入模式测试
    printf("测试2: 输入模式\r\n");
    DHT11_IO_IN();
    input_state = DHT11_DQ_IN;
    printf("  - 输入状态: %d\r\n", input_state);
    
    // 测试3: 多次重置尝试
    printf("测试3: 传感器响应测试\r\n");
    for(i = 0; i < 3; i++) {
        printf("  - 尝试 %d: ", i+1);
        DHT11_Rst();
        if(DHT11_Check() == 0) {
            printf("响应OK\r\n");
            return 0;  // 测试成功
        } else {
            printf("无响应\r\n");
        }
        delay_ms(100);  // 等待传感器稳定
    }
    
    printf("DHT11: 连接测试失败，传感器可能未连接或损坏\r\n");
    return 1;  // 测试失败
}

/**
 * @brief 数据有效性验证
 * @param temp 温度值
 * @param humi 湿度值
 * @return true-数据有效, false-数据无效
 */
bool DHT11_Validate_Data(u8 temp, u8 humi)
{
    static u8 last_temp = 25;
    static u8 last_humi = 50;
    static bool first_read = true;
    
    // DHT11规格范围检查
    if(temp > 50 || humi > 90 || humi < 20) {
        printf("DHT11: 数据超出规格范围 T=%d°C, H=%d%%\r\n", temp, humi);
        return false;
    }
    
    // 变化率检查（防止异常跳变）
    if(!first_read) {
        if(abs(temp - last_temp) > 10 || abs(humi - last_humi) > 20) {
            printf("DHT11: 数据变化过大 ΔT=%d, ΔH=%d\r\n", 
                   abs(temp - last_temp), abs(humi - last_humi));
            return false;
        }
    }
    
    last_temp = temp;
    last_humi = humi;
    first_read = false;
    
    return true;
}

/**
 * @brief 滑动平均滤波算法
 * @param new_data 新数据
 * @param buffer 数据缓冲区
 * @param size 缓冲区大小
 * @param index 当前索引指针
 * @param count 数据计数指针
 * @return 滤波后的数据
 */
float DHT11_Filter_Data(float new_data, float* buffer, u8 size, u8* index, u8* count)
{
    float sum = 0;
    u8 i;
    
    // 存储新数据
    buffer[*index] = new_data;
    *index = (*index + 1) % size;
    
    if(*count < size) {
        (*count)++;
    }
    
    // 计算平均值
    for(i = 0; i < *count; i++) {
        sum += buffer[i];
    }
    
    return sum / (*count);
}

/**
 * @brief 重置滤波器
 * @param filter 滤波器结构体指针
 */
void DHT11_Reset_Filter(DHT11_Filter_t* filter)
{
    u8 i;
    for(i = 0; i < DHT11_FILTER_SIZE; i++) {
        filter->temp_buffer[i] = 0;
        filter->humi_buffer[i] = 0;
    }
    filter->index = 0;
    filter->count = 0;
    filter->is_full = false;
    
    printf("DHT11: 滤波器已重置\r\n");
}

/**
 * @brief 设置校准偏移值
 * @param temp_offset 温度校准偏移
 * @param humi_offset 湿度校准偏移
 */
void DHT11_Set_Calibration(float temp_offset, float humi_offset)
{
    dht11_data.temp_offset = temp_offset;
    dht11_data.humi_offset = humi_offset;
    dht11_data.calibration_enabled = true;
    
    printf("DHT11: 校准设置 - 温度偏移:%.1f°C, 湿度偏移:%.1f%%\r\n", 
           temp_offset, humi_offset);
}

/**
 * @brief 获取DHT11数据句柄
 * @return DHT11数据结构体指针
 */
DHT11_Data_t* DHT11_Get_Data_Handle(void)
{
    return &dht11_data;
}

/**
 * @brief 增强版DHT11数据读取
 * @param data DHT11数据结构体指针
 * @return DHT11_Status_t 读取状态
 */
DHT11_Status_t DHT11_Read_Data_Enhanced(DHT11_Data_t* data)
{
    u8 temp_raw, humi_raw;
    float temp_filtered, humi_filtered;
    u8 result;
    u8 retry;
    u32 current_time = system_time_ms; // 获取当前时间
    u32 min_interval;
    
    // 根据模式设置最小读取间隔
    // 真实传感器需要至少1秒间隔，模拟模式可以更频繁
    min_interval = 1000; // 真实传感器1秒间隔
    
    // 检查读取间隔
    if(current_time - data->last_read_time < min_interval) {
        printf("DHT11: 读取间隔太短，需要等待 (模式:%s, 间隔:%dms)\r\n", 
               "真实", min_interval);
        return DHT11_ERROR_TIMEOUT;
    }
    
    // 多次重试读取
    for(retry = 0; retry < DHT11_MAX_RETRY; retry++) {
        result = DHT11_Read_Data(&temp_raw, &humi_raw);
        
        if(result == 0) { // 读取成功
            // 数据验证
            if(DHT11_Validate_Data(temp_raw, humi_raw)) {
                // 数据滤波
                temp_filtered = DHT11_Filter_Data((float)temp_raw, 
                                                  dht11_filter.temp_buffer, 
                                                  DHT11_FILTER_SIZE,
                                                  &dht11_filter.index,
                                                  &dht11_filter.count);
                
                humi_filtered = DHT11_Filter_Data((float)humi_raw, 
                                                  dht11_filter.humi_buffer, 
                                                  DHT11_FILTER_SIZE,
                                                  &dht11_filter.index,
                                                  &dht11_filter.count);
                
                // 应用校准
                if(data->calibration_enabled) {
                    temp_filtered += data->temp_offset;
                    humi_filtered += data->humi_offset;
                }
                
                // 更新数据
                data->temperature = temp_filtered;
                data->humidity = humi_filtered;
                data->status = DHT11_OK;
                data->retry_count = retry;
                data->last_read_time = current_time;
                
                printf("DHT11: 增强读取成功 - 原始T=%d°C,H=%d%% 滤波后T=%.1f°C,H=%.1f%% (重试%d次,%s)\r\n",
                       temp_raw, humi_raw, temp_filtered, humi_filtered, retry,
                       "真实模式");
                
                return DHT11_OK;
            } else {
                data->status = DHT11_ERROR_INVALID_DATA;
            }
        } else {
            data->status = (retry == DHT11_MAX_RETRY - 1) ? DHT11_ERROR_NO_RESPONSE : DHT11_ERROR_TIMEOUT;
        }
        
        delay_ms(500); // 重试间隔
    }
    
    data->retry_count = retry;
    printf("DHT11: 增强读取失败 - 重试%d次后仍无法获取有效数据 (%s)\r\n", retry,
           "真实模式");
    
    return data->status;
}

/**
 * @brief 带重试的DHT11数据更新
 */
void DHT11_Update_With_Retry(void)
{
    DHT11_Status_t status;
    static u8 consecutive_errors = 0;
    
    status = DHT11_Read_Data_Enhanced(&dht11_data);
    
    switch(status) {
        case DHT11_OK:
            consecutive_errors = 0;
            printf("DHT11: 数据更新成功\r\n");
            break;
            
        case DHT11_ERROR_TIMEOUT:
            consecutive_errors++;
            printf("DHT11: 读取超时\r\n");
            break;
            
        case DHT11_ERROR_INVALID_DATA:
            consecutive_errors++;
            printf("DHT11: 数据无效\r\n");
            break;
            
        case DHT11_ERROR_NO_RESPONSE:
            consecutive_errors++;
            printf("DHT11: 传感器无响应\r\n");
            break;
            
        default:
            consecutive_errors++;
            printf("DHT11: 未知错误\r\n");
            break;
    }
    
    // 连续错误处理
    if(consecutive_errors >= 5) {
        printf("DHT11: 连续%d次错误，重置滤波器并切换到模拟模式\r\n", consecutive_errors);
        DHT11_Reset_Filter(&dht11_filter);
        consecutive_errors = 0;
    }
}
