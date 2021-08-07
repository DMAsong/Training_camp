/*
 * colours_control.c
 *
 *  Created on: 2021年8月3日
 *      Author: Admin
 */
#include "ci112x_pwm.h"
#include "colours_control.h"
#include "ci112x_scu.h"
#include "ci_log.h"
#include "FreeRTOS.h"
#include "task.h"
#define MAX_DUTY 101
#define GCHANNEL PWM0
#define BCHANNEL PWM2
#define RCHANNEL PWM1
#define WCHANNEL PWM3
uint8_t light; //亮度值
uint8_t R, G, BL, WW;
uint8_t mode;
void my_colours_init(void)
{
	light = 100;
	R = 100;
	G = 0;
	BL = 0;
	WW = 0;

	pwm_init_t pwm0_config;
	Scu_SetDeviceGate(HAL_PWM0_BASE, ENABLE);
	Scu_SetIOReuse(PWM0_PAD, SECOND_FUNCTION);
	Scu_SetIOPull(PWM0_PAD, DISABLE); //关闭IO内部上下拉
	pwm0_config.freq = 1000;
	pwm0_config.duty = 0;
	pwm0_config.duty_max = MAX_DUTY;
	pwm_init(PWM0, pwm0_config);
	pwm_stop(PWM0);
	pwm_start(PWM0);

	pwm_init_t pwm1_config;
	Scu_SetDeviceGate(HAL_PWM1_BASE, ENABLE);
	Scu_SetIOReuse(PWM1_PAD, SECOND_FUNCTION);
	Scu_SetIOPull(PWM1_PAD, DISABLE); //关闭IO内部上下拉
	pwm1_config.freq = 1000;
	pwm1_config.duty = 0;
	pwm1_config.duty_max = MAX_DUTY;
	pwm_init(PWM1, pwm1_config);
	pwm_stop(PWM1);
	pwm_start(PWM1);

	pwm_init_t pwm2_config;
	Scu_SetDeviceGate(HAL_PWM2_BASE, ENABLE);
	Scu_SetIOReuse(PWM2_PAD, SECOND_FUNCTION);
	Scu_SetIOPull(PWM2_PAD, DISABLE); //关闭IO内部上下拉
	pwm2_config.freq = 1000;
	pwm2_config.duty = 0;
	pwm2_config.duty_max = MAX_DUTY;
	pwm_init(PWM2, pwm2_config);
	pwm_stop(PWM2);
	pwm_start(PWM2);

	pwm_init_t pwm3_config;
	Scu_SetDeviceGate(HAL_PWM3_BASE, ENABLE);
	Scu_SetIOReuse(PWM3_PAD, SECOND_FUNCTION);
	Scu_SetIOPull(PWM3_PAD, DISABLE); //关闭IO内部上下拉
	pwm3_config.freq = 1000;
	pwm3_config.duty = 0;
	pwm3_config.duty_max = MAX_DUTY;
	pwm_init(PWM3, pwm3_config);
	pwm_stop(PWM3);
	pwm_start(PWM3);

}
int set_light(uint8_t value)
{
	return (int)(value * light) / 100;
}
/*
*@brief 调节w通道占空比
*@param duty 占空比值
**/
void my_set_color(uint8_t Red, uint8_t Green, uint8_t Blue, uint8_t White)
{
	Red = set_light(Red);
	Green = set_light(Green);
	Blue = set_light(Blue);
	White = set_light(White);
	pwm_set_duty(WCHANNEL, White, MAX_DUTY);
	pwm_set_duty(RCHANNEL, Red, MAX_DUTY);
	pwm_set_duty(BCHANNEL, Blue, MAX_DUTY);
	pwm_set_duty(GCHANNEL, Green, MAX_DUTY);
}
/*
*@brief 关闭灯条
**/
void my_stop_control(void)
{
	pwm_stop(WCHANNEL);
	pwm_stop(RCHANNEL);
	pwm_stop(BCHANNEL);
	pwm_stop(GCHANNEL);
}
/*
*@brief 打开灯条
**/
void my_start_control(void)
{
	pwm_start(WCHANNEL);
	pwm_start(RCHANNEL);
	pwm_start(BCHANNEL);
	pwm_start(GCHANNEL);
}
void switch_color(uint8_t index)
{
	switch (index)
	{
	case 0:
		R = 100;
		G = 0;
		BL = 0;
		WW = 0;
		break;
	case 1:
		R = 0;
		G = 100;
		BL = 0;
		WW = 0;
		break;
	case 2:
		R = 0;
		G = 0;
		BL = 100;
		WW = 0;
		break;
	case 3:
		R = 100;
		G = 100;
		BL = 0;
		WW = 0;
		break;
	case 4:
		R = 0;
		G = 100;
		BL = 100;
		WW = 0;
		break;
	case 5:
		R = 100;
		G = 0;
		BL = 100;
		WW = 0;
		break;
	case 6:
		R = 100;
		G = 100;
		BL = 100;
		WW = 0;
		break;
	case 7:
		R = 0;
		G = 0;
		BL = 0;
		WW = 100;
		break;
	default:
		break;
	}
}
uint8_t scale8(uint8_t i, uint8_t scale)
{
    return (((uint16_t)i) * (1 + (uint16_t)(scale))) >> 8;
}
/*****************************************
*名  称:定制颜色呼吸模式
*参  数:颜色个数:C_number	RGB值:*color_buf
*备  注:
*****************************************/
void Breathe(void)
{
    static uint8_t breathUpDown = 1, breath = 0;
    static uint8_t color_index = 0;
    uint8_t tmpR, tmpG, tmpB;
    uint8_t d;

		if (color_index > 7)
		{
			color_index = 0;
		}
		switch_color(color_index);
		if (breathUpDown == 1)
		{
			breath = breath + 4;
			if (breath > 250)
				breathUpDown = 0;
		}
		else
    {
        breath = breath - 4;
        if (breath < 8)
        {
            breathUpDown = 1;
            color_index++;
        }
    }

    tmpR = scale8(R, breath);
    tmpG = scale8(G, breath);
    tmpB = scale8(BL, breath);
    my_set_color(tmpR, tmpG, tmpB,0);
}
void set_mode(uint8_t id)
{
	mode = id;
	colors_control_mode();
}
void colors_control_mode(void)
{
	switch (mode)
	{
	case OPEN_LIGHT:
		my_start_control();
		my_set_color(R, G, BL, WW);
		break;
	case CLOSE_LIGHT:
		my_set_color(0,0,0,0);
		my_stop_control();
		break;
	case LIGHT_MODE:
		switch_color(6);
		my_set_color(R, G, BL, WW);
		break;
	case COLORS_LIGHT:

		break;
	case RED_EFFECT:
		switch_color(0);
		my_set_color(R, G, BL, WW);
		break;
	case GREEN_EFFECT:
		switch_color(1);
		my_set_color(R, G, BL, WW);
		break;
	case BLUE_EFFECT:
		switch_color(2);
		my_set_color(R, G, BL, WW);
		break;
	case YELLOW_EFFECT:
		switch_color(3);
		my_set_color(R, G, BL, WW);
		break;
	case CYAN_EFFECT:
		switch_color(4);
		my_set_color(R, G, BL, WW);
		break;
	case PWRPLE_EFFECT:
		switch_color(5);
		my_set_color(R, G, BL, WW);
		break;
	case BREATH_EFFECT:
		Breathe();
		break;
	case ADD_BRIGHT:
		light +=20;
		if (light > 100)
		{
			light = 100;
		}
		my_set_color(R, G, BL, WW);
		break;
	case DEC_BRIGHT:
		if (light > 20)
		{
			light -=20;
		}

		my_set_color(R, G, BL, WW);
		break;
	case MEDIUM_BRIGHT:
		light = 50;
		my_set_color(R, G, BL, WW);
		break;
	case MAX_BRIGHT:
		light = 100;
		my_set_color(R, G, BL, WW);
		break;
	case MIN_BRIGHT:
		light = 5;
		my_set_color(R, G, BL, WW);
		break;
	case EYES_MODE:
		switch_color(7);
		my_set_color(R, G, BL, WW);
		break;

	default:
		break;
	}

}
