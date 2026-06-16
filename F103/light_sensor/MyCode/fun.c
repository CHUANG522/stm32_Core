#include "fun.h"
float leavel1,leavel2,leavel3,leavel4;
//printf重定向
int fputc(int ch,FILE *f)
{
    HAL_UART_Transmit(&huart1,(uint8_t *)&ch,1,0xffff);
    return ch;
}
int fgetc(FILE *f)
{
    uint8_t ch=0;
    HAL_UART_Receive(&huart1,&ch,1,0xffff);
    return ch;
}

void function(void)
{
	OLED_show();
	data_proc();
}
//测量PA3的模拟电压
float adc_get(void)
{
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 10);
	float temp;
	temp= HAL_ADC_GetValue(&hadc1);
	return temp *3.3f/4095 ;	
}
uint8_t OLED_buff[20];
uint32_t time_OLED;
uint32_t time;
void OLED_show(void)
{
	if(time_OLED%200 <100) return ;//0.1秒刷新	
	 
	sprintf((char *)OLED_buff,"adc:%.2f",adc_get());
	OLED_ShowString(1,1,(char *)OLED_buff);	
}
void data_proc(void)
{
	if(time_OLED%200 <100) return ;//借用OLED自增值0.1秒打印
//利用电压值分四个等级	Highlight   Normal  Shallow  Darkness
	if( adc_get() <0.8) printf(" Highlight\r\n ");
	if( adc_get() >0.8 &&adc_get() <1.6 ) printf(" Normal\r\n ");
	if( adc_get() >1.6 && adc_get() <2.4 ) printf(" Shallow\r\n ");
	if( adc_get() >2.4) printf(" Darkness\r\n ");
}
