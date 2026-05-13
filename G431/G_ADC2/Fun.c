#include "Head.h"
uint16_t ADC_Value1,ADC_Value2;
double Vol_Value1,Vol_Value2;
char string[20];
void LCD_Show (void)
{
	sprintf(string,"        Test       " ) ;
    LCD_DisplayStringLine(Line0 ,(uint8_t *)string);
	
    
	sprintf(string,"     Value1:%.2f  ", ADC_Vol(37)) ;
    LCD_DisplayStringLine(Line1 ,(uint8_t *)string);
	
	
	sprintf(string,"     Value2:%.2f  ", ADC_Vol(38)) ;
    LCD_DisplayStringLine(Line2 ,(uint8_t *)string);
}

double ADC_Vol (uint8_t ADC)
{
	if(ADC!=37 &&ADC!=38) return -1;
	double result;
	
    HAL_ADC_Start(&hadc1);
	ADC_Value1 =HAL_ADC_GetValue(&hadc1);
	Vol_Value1 =ADC_Value1*3.3/4096;
		
	HAL_ADC_Start(&hadc2);
	ADC_Value2 =HAL_ADC_GetValue(&hadc2);
	Vol_Value2 =ADC_Value2*3.3/4096;
	if(ADC==37)  result =Vol_Value2;
	else 		 result =Vol_Value1;
	return       result;
}
//接受字符函数
uint8_t Rec_temp ;
uint8_t Rec_arr[20];
uint8_t Send_arr[20];//可以不用这个
uint8_t Rec_Flag;
uint8_t counter;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
 if(huart ->Instance ==USART1) 
  {
	Rec_Flag=1;
    TIM2 ->CNT =0;
	 HAL_UART_Receive_IT (&huart1,&Rec_temp,1);
     Rec_arr[counter++] =Rec_temp;
  
  }
}
//处理接受字符函数
void Uart_Date_Rec(void)
{
	if(Rec_Flag)//标志位为1表示有数据传来
 {
  if(TIM2 ->CNT >15)//发送完成
   {
	   if(Rec_arr[0]=='l'&&Rec_arr[1]=='a'&&Rec_arr[2]=='n') 
       {
	  HAL_UART_Transmit(&huart1,(uint8_t *)Rec_arr,strlen((char *)Rec_arr),50); 
		      
	   }
	   else 
        {
		sprintf((char *)Send_arr,"error！");
		HAL_UART_Transmit(&huart1,(uint8_t *)Send_arr,strlen((char *)Send_arr),50); 
		}
		 Rec_Flag=0;
		memset(Rec_arr,0,counter);
		counter=0;
   }
 
 }
}
