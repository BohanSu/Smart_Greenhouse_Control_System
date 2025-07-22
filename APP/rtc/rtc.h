#ifndef __RTC_H__
#define __RTC_H__

#include "stm32f10x.h"

// 时间结构体
typedef struct
{
    u16 year;   // 年
    u8 month;   // 月
    u8 date;    // 日
    u8 hour;    // 时
    u8 min;     // 分
    u8 sec;     // 秒
    u8 week;    // 星期
} RTC_Time_t;

// 定时任务结构体
typedef struct
{
    u8 enable;          // 是否启用
    u8 hour;            // 触发小时
    u8 min;             // 触发分钟
    u8 repeat_type;     // 重复类型：0-一次性，1-每天，2-每周
    u8 week_mask;       // 星期掩码（bit0-6对应周一到周日）
    void (*callback)(void); // 回调函数
} RTC_Timer_t;

// 重复类型定义
#define TIMER_ONCE      0   // 一次性
#define TIMER_DAILY     1   // 每天
#define TIMER_WEEKLY    2   // 每周

// 星期定义
#define WEEK_MON        0x01
#define WEEK_TUE        0x02
#define WEEK_WED        0x04
#define WEEK_THU        0x08
#define WEEK_FRI        0x10
#define WEEK_SAT        0x20
#define WEEK_SUN        0x40

// 函数声明
u8 RTC_Init(void);                          // RTC初始化
void RTC_Set_Time(RTC_Time_t* time);        // 设置时间
void RTC_Get_Time(RTC_Time_t* time);        // 获取时间
u8 RTC_Set_Alarm(u8 hour, u8 min, u8 sec); // 设置闹钟
void RTC_Get_Week(RTC_Time_t* time);        // 计算星期
u32 RTC_Get_Counter(void);                  // 获取计数器值
void RTC_Set_Counter(u32 cnt);              // 设置计数器值

// 定时任务相关
void RTC_Timer_Init(void);                  // 定时任务初始化
u8 RTC_Timer_Add(RTC_Timer_t* timer);       // 添加定时任务
void RTC_Timer_Remove(u8 index);            // 移除定时任务
void RTC_Timer_Check(void);                 // 检查定时任务

// 工具函数
u8 RTC_Is_Leap_Year(u16 year);              // 判断闰年
u8 RTC_Get_Month_Days(u16 year, u8 month);  // 获取月份天数
void RTC_Format_Time(RTC_Time_t* time, char* str); // 格式化时间字符串
void RTC_Format_Date(RTC_Time_t* time, char* str); // 格式化日期字符串

// RTC中断处理函数（被stm32f10x_it.c调用）
void RTC_Process_Interrupt(void);

// 全局变量声明
extern RTC_Time_t current_time;

#endif /* __RTC_H__ */
 
