#include "fan_pwm.h"
#include "stdio.h"

static u16 pwm_period = 0;

/*******************************************************************************
* 函 数 名         : Fan_PWM_Init
* 函数功能		   : TIM3通道1 PWM初始化函数，用于风扇控制
* 输    入         : per:重装载值  psc:预分频系数
* 输    出         : 无
*******************************************************************************/
void Fan_PWM_Init(u16 per, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* 开启时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	
	/*  配置GPIO的模式和IO口 - 使用PA6引脚(TIM3_CH1) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; /* 复用推挽输出 */
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	pwm_period = per;
	
	TIM_TimeBaseInitStructure.TIM_Period = per;   /* 自动装载值 */
	TIM_TimeBaseInitStructure.TIM_Prescaler = psc; /* 预分频系数 */
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; /* 向上计数模式 */
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);	
	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; /* 高电平有效 */
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC1Init(TIM3, &TIM_OCInitStructure); /* 输出比较通道1初始化 */
	
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable); /* 使能TIMx在CCR1上的预装载寄存器 */
	TIM_ARRPreloadConfig(TIM3, ENABLE); /* 使能预装载寄存器 */
	
	TIM_Cmd(TIM3, ENABLE); /* 使能定时器 */
	
	/* 初始化时关闭风扇 */
	TIM_SetCompare1(TIM3, 0);
	
	printf("Fan PWM: Hardware PWM initialization complete (PA6, TIM3_CH1)\r\n");
}

/*******************************************************************************
* 函 数 名         : Fan_Set_Speed_Percent
* 函数功能		   : 设置风扇速度百分比
* 输    入         : percent: 速度百分比 0-100%
* 输    出         : 无
*******************************************************************************/
void Fan_Set_Speed_Percent(u8 percent)
{
	u32 ccr_val;

	if (percent > 100) {
		percent = 100;
	}
	
	if (pwm_period == 0) return;

	ccr_val = (pwm_period * percent) / 100;
	
	TIM_SetCompare1(TIM3, ccr_val);
	
	printf("Fan PWM: Speed set to %d%% (CCR=%d)\r\n", percent, ccr_val);
}

/*******************************************************************************
* 函 数 名         : Fan_Get_Speed_Percent
* 函数功能		   : 获取当前风扇速度百分比
* 输    入         : 无
* 输    出         : 速度百分比 0-100%
*******************************************************************************/
u8 Fan_Get_Speed_Percent(void)
{
	u32 ccr_val = TIM_GetCapture1(TIM3);
	
	if (pwm_period == 0) return 0;
	
	return (ccr_val * 100) / pwm_period;
}