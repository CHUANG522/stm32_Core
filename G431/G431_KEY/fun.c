#include <fun.h>

// 全局变量
int counter;
int counter_ms;
// led闪烁函数
void led_show(uint8_t led, uint8_t mode)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    if (mode)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
}

// 按键函数
//! B1_statu & B1_last 按键按下
//! B1_statu & !B1_last 按键长按
// B1_statu & !B1_last 按键松开
//!!!注意：三种状态必须独立，不能有if嵌套，因为只要进了一种判断if那么按键的状态就是一定的
uint8_t B1_statu = 1, B1_last = 1, B2_statu = 1, B2_last = 1, B3_statu = 1, B3_last = 1, B4_statu = 1, B4_last = 1;
uint16_t click_tim, click_tim_last;
uint8_t click_flag, click_count;
void key_scan(void)
{
    B1_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    B2_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    B3_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
    B4_statu = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    // 按键B1双击，第一次按下后先执行else函数，此时有了标志位=1，记录了第一次按下时间，再按下即可满足双击条件
    if (!B1_statu & B1_last) // 按键按下
    {
        click_tim = counter_ms;
        if (click_flag && (click_tim - click_tim_last < 500))
        {
            click_flag = 0;
            counter += 10;
        }
        else
        {
            click_flag = 1;
            click_tim_last = click_tim;
        }
    }

    // 按下即清零
    if (!B1_statu & !B1_last) // 按键长按
    {
    }
    if (B1_statu & !B1_last) // 按键松开
    {
    }

    // 按键B2
    if (B2_statu == 0 && B2_last == 1)
    {
    }
    // 按键B3
    if (B3_statu == 0 && B3_last == 1)
    {
    }
    // 按键B4
    if (B4_statu == 0 && B4_last == 1)
    {
    }
    B1_last = B1_statu;
    B2_last = B2_statu;
    B3_last = B3_statu;
    B4_last = B4_statu;
}
char string[20];
void lcd_show(void)
{

    sprintf(string, "  counter:%d       ", counter);
    LCD_DisplayStringLine(Line0, (uint8_t *)string);

    sprintf(string, "  counter_ms:%d    ", counter_ms);
    LCD_DisplayStringLine(Line1, (uint8_t *)string);

    sprintf(string, "  TIM2->CNT:%d     ", TIM2->CNT);
    LCD_DisplayStringLine(Line2, (uint8_t *)string);
}
// 定时器回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        counter_ms++;
    }
}
// main_proc在mian中重复执行的函数集合
void main_proc(void)
{
    lcd_show();

    key_scan();

    led_show(8, 1);
}
