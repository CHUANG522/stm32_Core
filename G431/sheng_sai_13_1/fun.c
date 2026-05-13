#include "fun.h"
uint8_t lcd_page ;
uint8_t data_R;                                       // 数据接收
uint32_t time_usart1;                                 // 滴答定时器记录usart1时间
int8_t B_current[3]={-1,-1,-1};                                 // 当前密码
uint8_t B_current_ture[3] = {1, 2, 3};                // 正确密码
uint8_t test_flag, led_flag, true_flag, false_flag_3,B_flag[3]={0}; // text=1开始校验，led闪烁，true=1判断密码正确，false=1输入三次都错
uint32_t time_CCR, time_led;                          // CCR记录输出CCR时的时间，led闪烁时间
uint8_t counter_test, counter_led;                    // test输入错误次数，led闪烁次数

void led_show(uint8_t led, uint8_t mode)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    if (mode)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
}
uint8_t B1_statu, B1_last, B2_statu, B2_last, B3_statu, B3_last, B4_statu, B4_last;
uint32_t key_time;

void key_scan(void)
{
    if (uwTick - key_time < 10)
        return;
    key_time = uwTick;
    B1_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    B2_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    B3_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
    B4_statu = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    if (!B1_statu & B1_last)
    {
        if (lcd_page == 0) // 注意这里不要依赖使用逻辑反！lcd_page，有三个界面不宜使用
            B_current[0]++;
		B_flag[0]=1;
    }

    if (!B2_statu & B2_last)
    {
        if (lcd_page == 0)
            B_current[1]++;
		B_flag[1]=1;
    }

    if (!B3_statu & B3_last)
    {
        if (lcd_page == 0)
            B_current[2]++;
		B_flag[2]=1;
    }

    if (!B4_statu & B4_last)
    {
        test_flag = 1;

    } // 输错回正部分
   // if (lcd_page == 2)
  //  { // 当在第三个界面时按下任意键回到输入界面
   //     if ((!B1_statu & B1_last) || (!B2_statu & B2_last) || (!B3_statu & B3_last))
     //       lcd_page = 0;
   // }
    B1_last = B1_statu;
    B2_last = B2_statu;
    B3_last = B3_statu;
    B4_last = B4_statu;
}
char string[20];
uint32_t lcd_time;
void lcd_show(void)
{
    if (uwTick - lcd_time < 99)
        return;
    lcd_time = uwTick;
    // 这里可以在全界面显示部分参数方便找逻辑错误
    //  sprintf(string, "   counter_led:%d    ", counter_led);
    //  LCD_DisplayStringLine(Line8, (uint8_t *)string);

    // sprintf(string, "   false_flag_3:%d    ", false_flag_3);
    // LCD_DisplayStringLine(Line9, (uint8_t *)string);

    // sprintf(string, "   time_led:%d    ", time_led);
    // LCD_DisplayStringLine(Line7, (uint8_t *)string);
    if (lcd_page == 0) // 输入界面
    {
        sprintf(string, "       PSD         ");
        LCD_DisplayStringLine(Line1, (uint8_t *)string);
         if(B_flag[0])
        sprintf(string, "    B1:%d          ", B_current[0]);
		 else
		 			 
		 sprintf(string, "    B1:@           ");
			
        LCD_DisplayStringLine(Line3, (uint8_t *)string);
         if(B_flag[1])
        sprintf(string, "    B2:%d          ", B_current[1]);
		 else 
		 sprintf(string, "    B2:@           ");
        LCD_DisplayStringLine(Line4, (uint8_t *)string);
         if(B_flag[2])
        sprintf(string, "    B3:%d          ", B_current[2]);
		 else 
		 sprintf(string, "    B3:@           ");
        LCD_DisplayStringLine(Line5, (uint8_t *)string);
    }
    if (lcd_page == 1) // 输出界面
    {
        sprintf(string, "       STA         ");
        LCD_DisplayStringLine(Line1, (uint8_t *)string);

        sprintf(string, "    F:%d           ", 8000000 / (TIM2->PSC + 1));
        LCD_DisplayStringLine(Line3, (uint8_t *)string);

        sprintf(string, "    D:%d%%         ", TIM2->CCR2);
        LCD_DisplayStringLine(Line4, (uint8_t *)string);
    }
  //  if (lcd_page == 2) // 输入错误界面
  //  {
   //     sprintf(string, "       PSD         ");
   //     LCD_DisplayStringLine(Line1, (uint8_t *)string);
//
    //    sprintf(string, "    B1:@          ");
    //    LCD_DisplayStringLine(Line3, (uint8_t *)string);

    //    sprintf(string, "    B2:@          ");
    //    LCD_DisplayStringLine(Line4, (uint8_t *)string);

    //    sprintf(string, "    B3:@          ");
     //   LCD_DisplayStringLine(Line5, (uint8_t *)string);
   // }
}
uint8_t string_R[20];
uint8_t counter_R, flag_R; // counter接收字符数，flag=1接收字符

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        HAL_UART_Transmit(&huart1, &data_R, 1, 50);
        string_R[counter_R] = data_R;
        // 注意counter_R加加写法，若整合写一定要注意后面的逻辑
        if (counter_R >= 4 && counter_R <= 6)
            B_current_ture[counter_R - 4] = data_R - 48;
        // 用数组定义参数的好处，这里逻辑更简单简洁
        counter_R++;

        flag_R = 1;
        time_usart1 = 0;
        HAL_UART_Receive_IT(&huart1, &data_R, 1); // 每此接收后开启接收中断等待下一次接收
    }
}

void data_R_proc(void)
{ // 字符串处理
    if (flag_R && time_usart1 > 15)
    { // 9600bit ，实际每个字符10位约1毫秒
        counter_R = flag_R = 0;
        memset(string_R, 0, 20); // 快速赋值函数memorey set
    }
}
void data_proc(void)
{ // 按键处理部分
    if (B_current[0] > 9)
        B_current[0] = 0;
    if (B_current[1] > 9)
        B_current[1] = 0;
    if (B_current[2] > 9)
        B_current[2] = 0;

    // 确认部分
    if (test_flag)
    {

        if (B_current[0] == B_current_ture[0] && B_current[1] == B_current_ture[1] && B_current[2] == B_current_ture[2])
        {
            true_flag = 1; // 正确！
            time_CCR = 0;
            LCD_Clear(Black);
            lcd_page = 1;
        }
        else
        {
            true_flag = 0;
           // lcd_page = 2;
			memset(B_flag,0,3);
			memset(B_current,-1,3);
            counter_test++;
            if (counter_test >= 3)
            {

                false_flag_3 = 1;
                // 记得增变量及时清零
                counter_test = 0;
             //   lcd_page = 2;
				memset(B_flag,0,3);
				memset(B_current,-1,3);
            }
        }
       
        test_flag = 0;
    }

    // 脉冲输出部分
    if (true_flag)
    {

        if (lcd_page)
        {
            if (time_CCR < 5000)
            {
                TIM2->CCR2 = 10;
                TIM2->PSC = 400 - 1;
                led_show(1, 1);
            }
            else
            {
                lcd_page = 0;
				memset(B_flag,0,3);
				memset(B_current,-1,3);
                TIM2->CCR2 = 100;
                TIM2->PSC = 800 - 1;
                 led_show(1, 0);       
                true_flag = 0; // 思考为什么不能放在200或211(尾)
            } // 这里如果放在202或214，因为tim_CCR是从0开始增，即提前把这个仅执行一次的部分终止了，根本不会执行到else部分
        }
    }

    // 错误三次处理
	
}
void main_proc(void)
{
    key_scan();
    lcd_show();
    data_proc();
    data_R_proc();
    led_show(8, 1);
    led_show(2, led_flag & false_flag_3);
}
