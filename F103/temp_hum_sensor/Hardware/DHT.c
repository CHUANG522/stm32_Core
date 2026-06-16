#include "DHT.h"
void DHT_out(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}
void DHT_in(void)
{
 GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

void DHT_start(void)
{
	DHT_out();
	
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_RESET);
	HAL_Delay(18);
	
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_SET);
	TIM3 ->CNT=0;//TIM3 1us计数,先清零后面进行延时
	while(TIM3 ->CNT <30) ;//信号0持续时间28us
	
	DHT_in();	
}

uint8_t  DHT_read(void)
{
 uint8_t data=0;
	for(uint8_t i=0;i<8;i++)
	{
		while(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_2) == 0);
		
		TIM3 ->CNT=0;//TIM3 1us计数,先清零后面进行延时
		while(TIM3 ->CNT <30) ;//信号0持续时间30us
		
		data <<=1;
		if( HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_2)== 1 ) 
		{
			data |= 1; //数据+1
		}
		while( HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_2) == 1 );
		
	}
  return data;
}
uint8_t R_data[4];
void DHT_R_data(void)
{
	uint8_t tem_H,tem_L,R_H,R_L,check;
	DHT_start();
	//HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_SET);
if( HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_2) == 0 ) //判断DHT11是否响应
	{
		while( HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_2) == 0); 
		while( HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_2) == 1);
		R_H = DHT_read();
		R_L = DHT_read();
		tem_H = DHT_read();
		tem_L = DHT_read();
		check = DHT_read();
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_RESET);
		
		TIM3 ->CNT=0;
		while(TIM3 ->CNT <55) ;//延时55us
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_SET);
		if(R_H + R_L + tem_H +tem_L == check)
		{
		R_data[0] =R_H;
		R_data[1] =R_L;
		R_data[2] =tem_H;
		R_data[3] =tem_L;			
		}
	}
	
	
	
}
