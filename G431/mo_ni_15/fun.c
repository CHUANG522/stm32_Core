#include "fun.h"

uint8_t lcd_page = 1;                    // lcd的界面页数
uint8_t T_F_flag = 1;                    // 显示周期or频率标志位
uint32_t capture_value1, capture_value2; // 输入捕获值
int fre1, fre2, TuA, TuB;                // 一定要大一些才行，不要设成8位的 要不然显示会出问题！！！//这里设为int 因为后面会和0比较
float freK1, freK2, TmA, TmB;            // 涉及小数设为浮点型freK为千频率，Tm为毫秒
int PH = 5000, PD = 1000, PX = 0;        // 题目的三个参数
uint8_t D_H_X_line;                      // 调试三参的line标志位
uint8_t NHA_counter, NHB_counter;
uint8_t NULL_flag;                                                                    // 出错标志位，1表示错误发生
uint8_t f1_P_flag, f1_P_flag_last, f2_P_flag_last, f2_P_flag;                         // 递增标志位
uint64_t counter_10ms, current_tim;                                                   // 定时器记时,获得一个时间差
uint16_t arrA[320] = {0}, arrB[320] = {0}, i, freA_min, freA_max, freB_min, freB_max; // 将数组设大一些防溢出
uint16_t NDA_counter, NDB_counter;                                                    // NDA/B计数
uint8_t NDA_counter_flag, NDA_counter_flag_last, NDB_counter_flag, NDB_counter_flag_last;
uint8_t flag_0 = 1; // 只执行一次
int f1, f2;         // 真实频率
void led_show(uint8_t led, uint8_t mode)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); // 开启锁存器

    if (mode)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); // 关闭锁存器
}

uint8_t B1_statu = 1, B1_last = 1, B2_statu = 1, B2_last = 1, B3_statu = 1, B3_last = 1, B4_statu = 1, B4_last = 1;
void key_scan(void)
{
    B1_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    B2_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    B3_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
    B4_statu = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    // 按键B1
    if (!B1_statu & B1_last)
    {
        if (lcd_page == 2)
        {
            // 按键1递增三个参数
            if (D_H_X_line == 0)
            {
                PD += 100;
                if (PD > 1000)
                    PD = 1000;
            }
            if (D_H_X_line == 1)
            {
                PH += 100;
                if (PH > 10000)
                    PH = 10000;
            }
            if (D_H_X_line == 2)
            {
                PX += 100;
                if (PX > 1000)
                    PX = 1000;
            }
        }
    }
    // 按键B2
    if (!B2_statu & B2_last)
    {
        if (lcd_page == 2)
        {
            // 按键2递减三个参数
            if (D_H_X_line == 0)
            {
                PD -= 100;
                if (PD < 100)
                    PD = 100;
            }
            if (D_H_X_line == 1)
            {
                PH -= 100;
                if (PH < 1000)
                    PH = 1000;
            }
            if (D_H_X_line == 2)
            {
                PX -= 100;
                if (PX < -1000)
                    PX = -1000;
            }
        }
    }
    // 按键B3
    if (!B3_statu & B3_last)
    {
        current_tim = counter_10ms; // 获取当前时间
        if (lcd_page == 1)
        {
            T_F_flag ^= 1; // 标志位每次取反
        }
        if (lcd_page == 2)
        {
            D_H_X_line++;
            if (D_H_X_line == 3)
            {
                D_H_X_line = 0;
            }
        }
    }
    if (lcd_page == 3)
    { // 长按逻辑清空数据
        if ((!B3_statu & !B3_last) && ((counter_10ms - current_tim) > 100))
        {
            NHA_counter = NHB_counter = 0;
            NDA_counter = NDB_counter = 0;
        }
    }
    // 按键B4
    if (!B4_statu & B4_last)
    {
        LCD_Clear(Black);
        lcd_page++;
        if (lcd_page == 3)
            T_F_flag = 1; // 在进入界面1时默认是F（频率调节模式）
        if (lcd_page == 4)
            lcd_page = 1; // 共四个界面
        if (lcd_page == 1)
            D_H_X_line = 0; // 进入界面2时默认是修改D参数
        if (lcd_page != 2)
    }
    B1_last = B1_statu;
    B2_last = B2_statu;
    B3_last = B3_statu;
    B4_last = B4_statu;
}
char string[20];

void lcd_show(void)
{
    if (lcd_page == 1)
    {
        if (NULL_flag == 1)
        { // 错误显示界面
            sprintf(string, "        NULL       ");
            LCD_DisplayStringLine(Line1, (uint8_t *)string);
        }
        else
        {
            if (T_F_flag)
            { // F，频率界面

                sprintf(string, "        DATA       ");
                LCD_DisplayStringLine(Line1, (uint8_t *)string);

                if (f1 < 1000)
                {
                    sprintf(string, "     A=%dHz        ", f1);
                    LCD_DisplayStringLine(Line3, (uint8_t *)string);
                }
                else
                {
                    sprintf(string, "     A=%.2fKHz     ", freK1);
                    LCD_DisplayStringLine(Line3, (uint8_t *)string);
                }
                if (f2 < 1000)
                {
                    sprintf(string, "     B=%dHz      ", f2);
                    LCD_DisplayStringLine(Line4, (uint8_t *)string);
                }
                else
                {
                    sprintf(string, "     B=%.2fKHz     ", freK2);
                    LCD_DisplayStringLine(Line4, (uint8_t *)string);
                }
            }

            if (!T_F_flag)
            { // T 周期界面
                sprintf(string, "        DATA       ");
                LCD_DisplayStringLine(Line1, (uint8_t *)string);
                if (TuA > 1000)
                {
                    sprintf(string, "     A=%.2fmS      ", TmA);
                    LCD_DisplayStringLine(Line3, (uint8_t *)string);
                }
                else
                {
                    sprintf(string, "     A=%duS        ", TuA);
                    LCD_DisplayStringLine(Line3, (uint8_t *)string);
                }

                if (TuB > 1000)
                {
                    sprintf(string, "     B=%.2fmS      ", TmB);
                    LCD_DisplayStringLine(Line4, (uint8_t *)string);
                }
                else
                {
                    sprintf(string, "     B=%duS        ", TuB);
                    LCD_DisplayStringLine(Line4, (uint8_t *)string);
                }
            }
        }
    }

    if (lcd_page == 2)
    { // 界面2 三参数界面高亮题目没说就不加
        sprintf(string, "        PARA       ");
        LCD_DisplayStringLine(Line1, (uint8_t *)string);

        sprintf(string, "     PD=%dHz         ", PD);
        LCD_DisplayStringLine(Line3, (uint8_t *)string);

        sprintf(string, "     PH=%dHz         ", PH);
        LCD_DisplayStringLine(Line4, (uint8_t *)string);

        sprintf(string, "     PX=%dHz         ", PX);
        LCD_DisplayStringLine(Line5, (uint8_t *)string);
    }
    if (lcd_page == 3)
    {
        sprintf(string, "        RECD       ");
        LCD_DisplayStringLine(Line1, (uint8_t *)string);

        sprintf(string, "     NDA=%d        ", NDA_counter);
        LCD_DisplayStringLine(Line3, (uint8_t *)string);

        sprintf(string, "     NDB=%d        ", NDB_counter);
        LCD_DisplayStringLine(Line4, (uint8_t *)string);

        sprintf(string, "     NHA=%d        ", NHA_counter);
        LCD_DisplayStringLine(Line5, (uint8_t *)string);

        sprintf(string, "     NHB=%d        ", NHB_counter);
        LCD_DisplayStringLine(Line6, (uint8_t *)string);
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{ // PWM捕获回调函数，勿忘使能定时器
    if (htim->Instance == TIM2)
    {
        capture_value1 = TIM2->CCR1; // 注意写成CCR1，指明通道，不能写成CCR
        TIM2->CNT = 0;
        fre1 = 1000000 / capture_value1;
    }
    if (htim->Instance == TIM16)
    {
        capture_value2 = TIM16->CCR1;
        TIM16->CNT = 0;
        fre2 = 1000000 / capture_value2;
    }
}
// 数据处理部分
void date_proc(void)
{
    // 处理频率
    if (f1 > 1000)
    {
        freK1 = 1.0 * f1 / 1000;
    }
    if (f2 > 1000)
    {
        freK2 = 1.0 * f2 / 1000;
    }
    // 处理周期
    TuA = (1.0 / f1) * 1000000; // 注意这里，1/fre是整形除法，直接为零了，应是1.0     capture_value1=(1.0 / fre1) * 1000000
    if (TuA > 1000)
        TmA = 1.0 * TuA / 1000;
    TuB = (1.0 / f2) * 1000000;
    if (TuB > 1000)
        TmB = 1.0 * TuB / 1000;
    // 处理NHA
    if (f1 > PH)
        f1_P_flag = 1;
    else
        f1_P_flag = 0;

    if (f1_P_flag & !f1_P_flag_last)
        // 防止一直递增，判断标志位实现递增
        NHA_counter++;

    f1_P_flag_last = f1_P_flag; // 一定注意循环体中last =current的放置位置
    // // 错误处理合并为一个
    // if ((f1) < 0)
    //     NULL_flagA = 1;
    // else
    //     NULL_flagA = 0;

    // 处理NHB
    if (f2 > PH)
        f2_P_flag = 1;
    else
        f2_P_flag = 0;

    if (f2_P_flag & !f2_P_flag_last)
        // 防止一直递增，判断标志位实现一次递增
        NHB_counter++;

    f2_P_flag_last = f2_P_flag;
    // 错误处理部分
    if (f2 < 0 || f1 < 0)
        NULL_flag = 1;
    else
        NULL_flag = 0;
    // 处理最大最小值部分,得到最大最小值
    if (i >= 300) // 这里不要写i==299时再执行以下部分，一定要写成>某个范围的数，这样更容易检测到
    {
        freA_max = arrA[100];
        freA_min = arrA[100];
        freB_max = arrB[100];
        freB_min = arrB[100];
        for (uint16_t j = 0; j < 300; j++)
        {

            if (arrA[j] > freA_max)
                freA_max = arrA[j];
            if (arrA[j] < freA_min)
                freA_min = arrA[j]; // 这里和初始化值密切相关，若min初始化为0，可能现象会不对
            if (arrB[j] > freB_max)
                freB_max = arrB[j];
            if (arrB[j] < freB_min)
                freB_min = arrB[j];
        }
        // 此部分仅执行一次，为了防止上电时就递增
        if (flag_0)
        {
            freA_min = freA_max;
            freB_min = freB_max;
            flag_0 = 0;
        }
        i = 0;
    }
    // 处理得到的最大最小值,注意只加一次
    // NDA部分
    if ((freA_max - freA_min) > PD)
        NDA_counter_flag = 1;
    else
        NDA_counter_flag = 0;
    if (NDA_counter_flag & !NDA_counter_flag_last)
        NDA_counter++;
    NDA_counter_flag_last = NDA_counter_flag;
    // NDB部分
    if ((freB_max - freB_min) > PD)
        NDB_counter_flag = 1;
    else
        NDB_counter_flag = 0;
    if (NDB_counter_flag & !NDB_counter_flag_last)
        NDB_counter++;
    NDB_counter_flag_last = NDB_counter_flag;
    // 处理真实频率
    f1 = fre1 + PX;
    f2 = fre2 + PX;
}

// 定时器回调部分
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
    {
        counter_10ms++;
        arrA[i] = f1;
        arrB[i] = f2;
        i++;
    }
}
// led综合亮灭部分
void led_show2(void)
{ // led亮灭的逻辑，逻辑运算
    led_show(1, lcd_page == 1);
    led_show(2, (f1) > PH);
    led_show(3, (f2) > PH);
    led_show(8, (NDB_counter > 2 || NDA_counter > 2));
}
// main主函数 循环部分
void main_proc(void)
{
    lcd_show();
    key_scan();
    date_proc();
    led_show2();
}
