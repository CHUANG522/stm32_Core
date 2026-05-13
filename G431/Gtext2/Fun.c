#include "fun.h"
uint8_t B1_statu = 1, B1_last = 1;
uint8_t counter = 1;
uint32_t fre = 1;
uint8_t led_flag_1, led_flag_2, led_flag_3, led_start;
uint32_t counter_ms, counter_100ms;
void led_show(uint8_t led, uint8_t mode)
{
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
	if (mode)
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_RESET);
	else
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
}

void key_scan(void)

{
	B1_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
	if ((!B1_statu) & B1_last)
	{
	}

	B1_last = B1_statu;
}
char string[20];
void lcd_show(void)
{
	sprintf(string, "123456789");
	LCD_DisplayStringLine(Line0, (uint8_t *)string);

	sprintf(string, "        DATA       ");
	LCD_DisplayStringLine(Line3, (uint8_t *)string);

	sprintf(string, "     VR37:%.2fV    ", adc_get());
	LCD_DisplayStringLine(Line4, (uint8_t *)string);

	sprintf(string, "    fre:%dHz   ", 1000000 / TIM2->ARR);
	LCD_DisplayStringLine(Line8, (uint8_t *)string);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM17)
	{
		counter_ms++;
	}
}

uint8_t data;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		HAL_UART_Receive_IT(&huart1, &data, 1);
		if (data == 'X')
		{
			led_flag_1 ^= 1;
		}
		else if (data == 'Y')
		{
			led_flag_2 ^= 1;
		}
		else
		{
			led_start = 1;
		}
		HAL_UART_Transmit(&huart1, &data, 1, 50);
	}
}
void led_SHOW(void)
{
	led_show(1, led_flag_1);
	led_show(2, led_flag_2);
	if (led_start)
		led_show(8, led_flag_3);
}
float adc_get(void)
{
	HAL_ADC_Start(&hadc2); // 注意每次调用
	return HAL_ADC_GetValue(&hadc2) * 3.3 / 4095;
}
void data_proc(void)
{
	// led闪烁部分
	if (counter_ms > 100 && led_start)
	{
		counter_ms = 0;
		led_flag_3 ^= 1;
		counter_100ms++;
	}
	if (counter_100ms > 50)
	{
		counter_100ms = 0;
		led_start = 0;
	}
	// adc控制PA1输出频率部分
	if (adc_get() > 2.4)
		TIM2->ARR = 1000 - 1;
	else
		TIM2->ARR = 10000 / 6;
}
void main_proc(void)
{

	lcd_show();
	adc_get();
	data_proc();
	led_SHOW();
}
