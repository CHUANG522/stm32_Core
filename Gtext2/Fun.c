//头文件
#include "Head.h" 

//全局变量
int counter=0;
uint8_t  LED_Mode=0;//LED的亮灭，%2 可控制在0，1之间
uint8_t  High_Line =0;//LCD某行的高亮显示，除于24等于行数
uint32_t  fre,capture_value ; //输入捕获的频率

//宏定义	
#define Sys_fre (80000000) //系统时钟
#define Psc     (80-1)     //预分频
#define High_Show_Num (5) //高亮显示行数

//实现任意LED开灭
void LED_Show(uint8_t LED,uint8_t mode)
{ 
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);//开🔒将指定LED点亮或熄灭	
	
	
	if(mode ==1)
	HAL_GPIO_WritePin( GPIOC,GPIO_PIN_8  <<(LED -1),GPIO_PIN_RESET);
	else 
	HAL_GPIO_WritePin( GPIOC,GPIO_PIN_8 <<(LED -1),GPIO_PIN_SET);	
	
		
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);//关🔒
}
//按键封装
 //按键状态变量：如果没有对Key的状态赋初始值,那么默认为0
uint8_t Key_B1 =1;
uint8_t Key_B1_Last =1;
uint8_t Key_B2 =1;
uint8_t Key_B2_Last=1 ;
uint8_t Key_B3 =1;
uint8_t Key_B3_Last=1 ;

void Key_Scan(void)
{  //按键B1
	Key_B1 =  HAL_GPIO_ReadPin (GPIOB,GPIO_PIN_0);//先读取按键的实时电平
		//电平变化逻辑（这一次电平和上一次不同）
	if(Key_B1 !=Key_B1_Last )// 检测到B1电平发生变化，执行B1的操作
 {
	HAL_Delay(10);// 延时消抖：等待抖动消失
    Key_B1 =  HAL_GPIO_ReadPin (GPIOB,GPIO_PIN_0);//稳定按键再次读取相应的电平	
	
	//if判断条件的原理：按键按下则此时是底电平，其上一次一定是高电平
  if(Key_B1 == 0&& Key_B1_Last ==1)  TIM3 -> CNT =0; //按键B1按下 按下时应使CNT归零
  else  if (Key_B1 == 1&& Key_B1_Last ==0 &&TIM3 -> CNT <15000) //按键B1松开短按	 
            counter --;
       
	 
	 Key_B1_Last =Key_B1; //得到key的上一个状态
 }	
 
 //长按逻辑（电平不变逻辑）
 if(Key_B1 == 0&& Key_B1_Last ==0)  //按键B1一直按着
 {  HAL_Delay(10);
	Key_B1 =  HAL_GPIO_ReadPin (GPIOB,GPIO_PIN_0);
	if(TIM3 -> CNT >=15000) //按键B1长按	
    counter ++;		
	
	Key_B1_Last =Key_B1;  
 }	
	//按键B2
    Key_B2 =  HAL_GPIO_ReadPin (GPIOB,GPIO_PIN_1);
	if(Key_B2 !=  Key_B2_Last)// 检测到B2电平发生变化，执行B2的操作
 {
	HAL_Delay(10);
	Key_B2 =  HAL_GPIO_ReadPin (GPIOB,GPIO_PIN_1);
	if(Key_B2 == 0&& Key_B2_Last ==1) counter--;
	Key_B2_Last =Key_B2;
 }	
	//按键B3
    Key_B3 =  HAL_GPIO_ReadPin (GPIOB,GPIO_PIN_2);
	if(Key_B3 != Key_B3_Last)// 检测到B3电平发生变化，执行B3的操作
 {
	HAL_Delay(10);
	Key_B3 =  HAL_GPIO_ReadPin (GPIOB,GPIO_PIN_2);
	if(Key_B3 == 0&& Key_B3_Last==1) //按键三按下
	{
		High_Line = (High_Line +1) % High_Show_Num; //高亮显示行：自增取模写法
	
	
	 LCD_SetTextColor(White);	
	}
	 Key_B3_Last =Key_B3;
 }	
	
}
//                              按键的通用逻辑
//1：设变量赋值（若不赋值则默认为零）
//2：在两种if逻辑（电平变与不变）之前先读取按键状态
//3：确定if逻辑后延时10ms后，再次读取稳定电平
//4：在逻辑if最后将 Key_B_Last =Key_B;

//LCD显示以及高亮
char string[20];

void LCD_Show(void)
{ 
	if(High_Line ==0) LCD_SetTextColor(Yellow);
	else                  LCD_SetTextColor(White);
	sprintf(string, "        text       ");
	LCD_DisplayStringLine ( Line0 , (uint8_t *)string);
	
	if(High_Line ==1) LCD_SetTextColor(Red);
	else                  LCD_SetTextColor(White);
	sprintf(string ,"     counter:%d    ",counter);
	LCD_DisplayStringLine ( Line1 , (uint8_t *)string);
	
	if(High_Line ==2) LCD_SetTextColor(Blue);
	else                  LCD_SetTextColor(White);
	sprintf(string ,"     CNT:%d    ",TIM3 -> CNT);
	LCD_DisplayStringLine ( Line2 , (uint8_t *)string);
	
	if(High_Line ==3) LCD_SetTextColor(Blue2);
	else                  LCD_SetTextColor(White);
	sprintf(string ,"     High_Line:%d   ",High_Line);
	LCD_DisplayStringLine ( Line3 , (uint8_t *)string);
	
	if(High_Line ==4) LCD_SetTextColor(Green);
	else                  LCD_SetTextColor(White);
	sprintf(string ,"     fre:%d   ",fre);
	LCD_DisplayStringLine ( Line4 , (uint8_t *)string);
}

//时钟控制LED亮灭
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim -> Instance == TIM2)
  {
  LED_Mode ++;
  LED_Mode %=2;  
  
  }
}
//输入捕获，PWM频率
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if(htim -> Instance == TIM17)
	{
		//capture_value = HAL_TIM_ReadCapturedValue (&htim17,TIM_CHANNEL_1);//此函数在捕获到上升沿时，会将CNT赋值给CCR
	capture_value = TIM17 ->CCR1 ; //也可以这样写
	TIM17 ->CNT =0;
	fre = Sys_fre/((Psc +1) * capture_value) ;
	}




}
/*测量ADC的电压值
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
*/
