#include "fun.h"
#define PI 3.1415926

uint8_t lcd_page;
uint32_t fre;
float vol,vol_last;
uint8_t low_high,high_low=1,low_high_flag,low_high_counter;
uint32_t time_vol;
uint32_t time_fre;
uint8_t time_fre_counter;
uint8_t R_K[2] ={1,1};
float V;
uint8_t B2_flag;
uint32_t time_B2;
uint8_t flag_1=1;//	仅执行一次标志位
void led_show(uint8_t led,uint8_t mode)
{
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
  if(mode)
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 <<(led-1), GPIO_PIN_RESET);
  else
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 <<(led-1), GPIO_PIN_SET);
	  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);

}

uint8_t B1_statu,B1_last, B2_statu,B2_last, B3_statu,B3_last, B4_statu,B4_last;
uint32_t time_key;
void key_scan(void)
{
	if(uwTick - time_key <9) return;
	else time_key =uwTick;
    B1_statu=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0);
	B2_statu=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1);
	B3_statu=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2);
	B4_statu=HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);

	if(!B1_statu & B1_last )
	{	 
	  if(++lcd_page>2) lcd_page=0;
	  LCD_Clear(Black);
	}
	
	if(!B2_statu & B2_last )
	{
		if(flag_1)
		{
			B2_flag=1;
	 		low_high^=1;
	 		high_low^=1;
	 		time_fre_counter=0;
			flag_1=0;
		}
		if(time_B2 >5000)
	 {
	 B2_flag=1;
	 low_high^=1;
	 high_low^=1;
	 time_fre_counter=0;
	 }
	 time_B2=0;
	}
	if(!B3_statu & B3_last )
	{
	
	}
	if(!B4_statu & B4_last )
	{
	
	}
    B1_last=B1_statu;
	B2_last=B2_statu;
	B3_last=B3_statu;
	B4_last=B4_statu;
}
uint8_t string[20];
uint32_t time_lcd;
void lcd_show(void)
{
  if(uwTick - time_lcd <99) return;
  else time_lcd = uwTick;
  
	// sprintf((char *)string ," vol:%.1f  ",vol);
	//   LCD_DisplayStringLine(Line6,string);
		
	 sprintf((char *)string ,"low_high:%d",low_high);
	   LCD_DisplayStringLine(Line7,string);

	   sprintf((char *)string ,"high_low:%d",high_low);
	   LCD_DisplayStringLine(Line8,string);

	// sprintf((char *)string ,"TIM2 -> PSC:%d",TIM2 -> PSC);
	//   LCD_DisplayStringLine(Line9,string);

	if(lcd_page==0)
	{
	  sprintf((char *)string ,"123456789           ");
	  LCD_DisplayStringLine(Line0,string);
		
	  sprintf((char *)string ,"        DATA        ");
	  LCD_DisplayStringLine(Line1,string);
		if(low_high_flag)
	  sprintf((char *)string ,"     M=H            ");
		else
	  sprintf((char *)string ,"     M=L            ");	
	  LCD_DisplayStringLine(Line3,string);
		
	  sprintf((char *)string ,"     P=%d%%         ",TIM2->CCR2);
	  LCD_DisplayStringLine(Line4,string);	
		
	  sprintf((char *)string ,"     V=%.1f         ",V);
	  LCD_DisplayStringLine(Line5,string);		
	}
    if(lcd_page==1)
	{
	 
	  sprintf((char *)string ,"        PARA        ");
	  LCD_DisplayStringLine(Line1,string);

	  sprintf((char *)string ,"     R=             ");
	  LCD_DisplayStringLine(Line3,string);
		
	  sprintf((char *)string ,"     K=             ");
	  LCD_DisplayStringLine(Line4,string);	 
	}
    if(lcd_page==2)
	{
	 
	  sprintf((char *)string ,"        RECD        ");
	  LCD_DisplayStringLine(Line1,string);

	  sprintf((char *)string ,"     N=             ");
	  LCD_DisplayStringLine(Line3,string);
		
	  sprintf((char *)string ,"     MH=            ");
	  LCD_DisplayStringLine(Line4,string);	
		
	  sprintf((char *)string ,"     ML=            ");
	  LCD_DisplayStringLine(Line5,string);		
	}
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if(htim ->Instance ==TIM17)
  {
     TIM17 ->CNT =0;
	 fre = 1000000/( HAL_TIM_ReadCapturedValue(&htim17, TIM_CHANNEL_1) );	  
  }


}

void ADC_vol(void)
{	
		HAL_ADC_Start(&hadc2);
		vol = HAL_ADC_GetValue(&hadc2) *3.3 /4095;	
        
		// if(vol_last <3 && vol >3)	
		// 	{low_high=1; 	time_fre=0;}
	
		// if(vol_last>1 && vol <1)	
		// 	{high_low =1; 	time_fre=0;}

		// if( uwTick -time_vol > 9)		
		// {
		// 	vol_last =vol;
		// 	time_vol=uwTick;
		// }
}
void data_proc(void)
{
	//占空比输出处理
	if(vol <1) 
	{
		TIM2 -> CCR2 =10;
	//	TIM2 -> PSC = 200-1;
	}
	else if(vol >3)
		{
		 TIM2 -> CCR2 =85;
	//	 TIM2 -> PSC = 100-1;
		}
	else 
	{
		TIM2 -> CCR2 = (37.5f * vol -27.5f);
        
	}	
	if(low_high)
	{
		TIM2 -> PSC =10000/(time_fre_counter +50) -1;
		low_high_flag=1;
	}
	if(high_low)
	{   low_high_flag=0;
		TIM2 -> PSC =10000/( 50-time_fre_counter +50)-1 +5;
	}
	//频率显示
	fre =800000/(TIM2 ->PSC +1);
	//V处理
	V = 2 * PI *fre * R_K[0] / ( 100*R_K[1] );
}
void main_proc(void)
{
	lcd_show();
	key_scan();
	data_proc();
	ADC_vol();
  led_show(8,1);
	HAL_Delay();

}
