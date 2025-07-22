#include "rtc.h"
#include "SysTick.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"

// 全局变量
RTC_Time_t current_time;
static RTC_Timer_t rtc_timers[8];  // 最多支持8个定时任务
static u8 timer_count = 0;

// 月份天数表（平年）
static const u8 month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// RTC初始化
u8 RTC_Init(void)
{
    // 启用PWR和BKP时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    
    // 允许访问备份域
    PWR_BackupAccessCmd(ENABLE);
    
    // 检查是否首次上电
    if(BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    {
        // 首次上电，初始化RTC
        printf("RTC First Configuration...\r\n");
        
        // 复位备份域
        BKP_DeInit();
        
        // 启用LSE振荡器
        RCC_LSEConfig(RCC_LSE_ON);
        
        // 等待LSE稳定
        while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
        {
            // 超时处理
            static u16 timeout = 0;
            if(++timeout > 5000)
            {
                printf("LSE Timeout, Using LSI\r\n");
                // 使用LSI作为备选
                RCC_LSEConfig(RCC_LSE_OFF);
                RCC_LSICmd(ENABLE);
                while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
                RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
                break;
            }
            delay_ms(1);
        }
        
        if(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == SET)
        {
            // LSE启动成功
            RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        }
        
        // 启用RTC时钟
        RCC_RTCCLKCmd(ENABLE);
        
        // 等待RTC同步
        RTC_WaitForSynchro();
        
        // 等待上次写操作完成
        RTC_WaitForLastTask();
        
        // 设置RTC中断
        RTC_ITConfig(RTC_IT_SEC, ENABLE);
        
        // 等待上次写操作完成
        RTC_WaitForLastTask();
        
        // 设置分频系数，32768-1，1s中断一次
        RTC_SetPrescaler(32767);
        
        // 等待上次写操作完成
        RTC_WaitForLastTask();
        
        // 设置初始时间：2025-01-01 00:00:00
        RTC_Time_t init_time = {2025, 1, 1, 0, 0, 0, 0};
        RTC_Set_Time(&init_time);
        
        // 写入标志，表示RTC已配置
        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
        
        printf("RTC Configuration Complete\r\n");
    }
    else
    {
        // RTC已经配置过
        printf("RTC Already Configured, Waiting Sync...\r\n");
        
        // 等待RTC同步
        RTC_WaitForSynchro();
        
        // 启用秒中断
        RTC_ITConfig(RTC_IT_SEC, ENABLE);
        
        // 等待上次写操作完成
        RTC_WaitForLastTask();
        
        printf("RTC Sync Complete\r\n");
    }
    
    // 配置NVIC
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 初始化定时任务
    RTC_Timer_Init();
    
    return 0;
}

// 设置时间
void RTC_Set_Time(RTC_Time_t* time)
{
    u32 seccount = 0;
    u32 temp = 0;
    u16 t;
    
    // 计算从1970年1月1日到设定时间的秒数
    for(t = 1970; t < time->year; t++)
    {
        if(RTC_Is_Leap_Year(t))
            seccount += 31622400; // 闰年秒数
        else 
            seccount += 31536000; // 平年秒数
    }
    
    // 计算月份
    for(t = 0; t < time->month - 1; t++)
    {
        temp = RTC_Get_Month_Days(time->year, t + 1);
        seccount += temp * 86400; // 月份的秒数
    }
    
    seccount += (time->date - 1) * 86400; // 日期秒数
    seccount += time->hour * 3600;        // 小时秒数
    seccount += time->min * 60;           // 分钟秒数
    seccount += time->sec;                // 秒数
    
    RTC_Set_Counter(seccount);
    
    // 更新当前时间
    current_time = *time;
    RTC_Get_Week(&current_time);
}

// 获取时间
void RTC_Get_Time(RTC_Time_t* time)
{
    static u16 days_month[13] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    u32 seccount = RTC_Get_Counter();
    u32 temp = 0;
    u16 temp1 = 0;
    
    temp = seccount / 86400; // 得到天数
    
    if(days_month[0] == 0)
    {
        // 重新计算days_month
        for(temp1 = 1; temp1 < 13; temp1++)
        {
            days_month[temp1] = days_month[temp1-1] + RTC_Get_Month_Days(1970, temp1);
        }
    }
    
    time->year = 1970; // 从1970年开始
    
    while(temp >= 365)
    {
        if(RTC_Is_Leap_Year(time->year))
        {
            if(temp >= 366) temp -= 366;
            else break;
        }
        else temp -= 365;
        time->year++;
    }
    
    // 计算月份
    temp1 = 0;
    while(temp1 < 12)
    {
        if(RTC_Is_Leap_Year(time->year) && temp1 == 1)
        {
            if(temp < (days_month[temp1+1] + 1)) break;
        }
        else 
        {
            if(temp < days_month[temp1+1]) break;
        }
        temp1++;
    }
    
    time->month = temp1 + 1;
    time->date = temp - days_month[temp1] + 1;
    
    temp = seccount % 86400; // 得到秒数
    time->hour = temp / 3600;
    time->min = (temp % 3600) / 60;
    time->sec = (temp % 3600) % 60;
    
    RTC_Get_Week(time);
}

// 设置闹钟
u8 RTC_Set_Alarm(u8 hour, u8 min, u8 sec)
{
    u32 seccount = 0;
    RTC_Time_t temp_time;
    
    RTC_Get_Time(&temp_time);
    
    seccount = temp_time.hour * 3600 + temp_time.min * 60 + temp_time.sec;
    seccount = hour * 3600 + min * 60 + sec - seccount;
    
    if(seccount <= 0) seccount += 86400; // 如果是明天的闹钟
    
    RTC_WaitForLastTask();
    RTC_SetAlarm(RTC_GetCounter() + seccount);
    RTC_WaitForLastTask();
    RTC_ITConfig(RTC_IT_ALR, ENABLE);
    RTC_WaitForLastTask();
    
    return 0;
}

// 计算星期
void RTC_Get_Week(RTC_Time_t* time)
{
    u16 temp2;
    u8 yearH, yearL;
    
    yearH = time->year / 100;
    yearL = time->year % 100;
    
    if(time->month < 3)
    {
        yearL--;
        time->month += 12;
    }
    
    temp2 = (yearL + yearL/4 + yearH/4 - 2*yearH + 26*(time->month+1)/10 + time->date - 1);
    time->week = temp2 % 7;
}

// 获取计数器值
u32 RTC_Get_Counter(void)
{
    return RTC_GetCounter();
}

// 设置计数器值
void RTC_Set_Counter(u32 cnt)
{
    RTC_WaitForLastTask();
    RTC_SetCounter(cnt);
    RTC_WaitForLastTask();
}

// 判断闰年
u8 RTC_Is_Leap_Year(u16 year)
{
    if(year % 4 == 0)
    {
        if(year % 100 == 0)
        {
            if(year % 400 == 0) return 1;
            else return 0;
        }
        else return 1;
    }
    else return 0;
}

// 获取月份天数
u8 RTC_Get_Month_Days(u16 year, u8 month)
{
    u8 days = month_days[month - 1];
    if(month == 2 && RTC_Is_Leap_Year(year))
        days = 29;
    return days;
}

// 格式化时间字符串
void RTC_Format_Time(RTC_Time_t* time, char* str)
{
    sprintf(str, "%02d:%02d:%02d", time->hour, time->min, time->sec);
}

// 格式化日期字符串
void RTC_Format_Date(RTC_Time_t* time, char* str)
{
    const char* week_str[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    sprintf(str, "%04d-%02d-%02d %s", 
            time->year, time->month, time->date, week_str[time->week]);
}

// 定时任务初始化
void RTC_Timer_Init(void)
{
    timer_count = 0;
    memset(rtc_timers, 0, sizeof(rtc_timers));
}

// 添加定时任务
u8 RTC_Timer_Add(RTC_Timer_t* timer)
{
    if(timer_count >= 8) return 1; // 定时任务已满
    
    rtc_timers[timer_count] = *timer;
    timer_count++;
    
    printf("Add Timer Task: %02d:%02d\r\n", timer->hour, timer->min);
    return 0;
}

// 移除定时任务
void RTC_Timer_Remove(u8 index)
{
    if(index >= timer_count) return;
    
    // 移动后续任务
    for(u8 i = index; i < timer_count - 1; i++)
    {
        rtc_timers[i] = rtc_timers[i + 1];
    }
    timer_count--;
}

// 检查定时任务
void RTC_Timer_Check(void)
{
    u8 i;
    for(i = 0; i < timer_count; i++)
    {
        if(rtc_timers[i].active)
        {
            if(rtc_timers[i].counter > 0)
            {
                rtc_timers[i].counter--;
                if(rtc_timers[i].counter == 0)
                {
                    if(rtc_timers[i].callback)
                    {
                        rtc_timers[i].callback();
                    }
                    if(rtc_timers[i].mode == TIMER_ONE_SHOT)
                    {
                        rtc_timers[i].active = 0;
                    }
                    else
                    {
                        rtc_timers[i].counter = rtc_timers[i].period;
                    }
                }
            }
        }
    }
}

// RTC中断处理函数 - 由RTC_IRQHandler调用
void RTC_Process_Interrupt(void)
{
    // 从RTC硬件读取当前时间到全局变量
    RTC_Get_Time(&current_time);

    // 检查并执行定时任务
    RTC_Timer_Check();
} 