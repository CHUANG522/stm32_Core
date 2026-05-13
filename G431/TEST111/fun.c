#include "fun.h"
uint8_t lcd_page;  // lcd界面
int16_t counter;
uint8_t flag_double_click;  // 双击标志位
uint8_t data_R;             // USART接受一个字符
uint8_t arr_data_R[20], arr_data_T[20], counter_data_R, flag_R;
// 依次为 USART接收数组，发送数组，接收字符数，接收成功标志位
float vol_37, vol_38;  // 模拟电压值
uint32_t fre1, fre2;   // 输入捕获频率
uint8_t eeprom_data;   // eeprom 写读数据
uint32_t time;
led_shan led8 = {0, 0, 0, 0};  // 初始化结构体变量

void led_show(uint8_t led, uint8_t mode)
{  // 勿忘开启PD2 锁存开关
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
    if (mode)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
}
uint8_t B1_statu, B1_last, B2_statu, B2_last, B3_statu, B3_last, B4_statu, B4_last;
uint32_t time_dalay_key;  // 按键消抖延时
uint32_t time_B1, time_B2, time_B2_last;
// 根据滴答定时器记录按键按下或上一次的时间
void key_scan(void)
{
    if (uwTick - time_dalay_key < 9) return;  // uwTick是向上计数
    time_dalay_key = uwTick;

    B1_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
    B2_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
    B3_statu = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
    B4_statu = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    if (B1_statu == 0 && B1_last)  // 按下
    {
        time_B1 = 0;
        counter++;
    }

    if ((B1_statu && B1_last == 0) && time_B1 > 1000)  // 长按
    {
    }

    if (B2_statu == 0 && B2_last)  // 双击
    {                              // 双击逻辑，每次按下获取当前时间，两次时间差小于某个值视为双击
        // 因为按下就要执行对应的逻辑(要么单击逻辑要么双击逻辑) 故使用if else来执行相应操作
        counter = 0;
        time_B2 = uwTick;  // 第一次按下记录当前时间
        counter++;
        flag_double_click = 1;  // 双击标志位置1
        // 采用if else
        // 执行双击逻辑，每次按下都用last记录当前时间，当第二次按下时time_B2是一个全新的值，即计算差值即可
        if (flag_double_click && (time_B2 - time_B2_last) < 300)
        {
            flag_double_click = 0;
            counter -= 10;
        }
        else
        {
            time_B2_last = time_B2;
        }
    }
    // 双击

    B1_last = B1_statu;
    B2_last = B2_statu;
    B3_last = B3_statu;
    B4_last = B4_statu;
}
uint8_t string[20];
uint32_t time_lcd;  // lcd刷新率时间
void lcd_show(void)
{
    if (uwTick - time_lcd < 99)  // 每0.1秒刷新
        return;
    else
        time_lcd = uwTick;

    if (lcd_page == 0)
    {
        sprintf((char*) string, "Hello world!");
        LCD_DisplayStringLine(Line0, string);

        sprintf((char*) string, "   time:%d  ", time % 1000 + 1);
        LCD_DisplayStringLine(Line1, string);

        sprintf((char*) string, "time_B1:%d   ", time_B1);
        LCD_DisplayStringLine(Line2, string);

        sprintf((char*) string, "time_data_R :%d", time_data_R);
        LCD_DisplayStringLine(Line3, string);

        sprintf((char*) string, " sd   ");
        LCD_DisplayStringLine(Line4, string);
    }
}
uint32_t time_data_R;  // USART计时器
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART1)
    {
        HAL_UART_Transmit(&huart1, &data_R, 1, 50);

        time_data_R = 0;
        flag_R = 1;
        arr_data_R[counter_data_R++] = data_R;
        // 每次中断要开启
        HAL_UART_Receive_IT(&huart1, &data_R, 1);
    }
}
void U_data_R_proc(void)
{  // 串口接收字符串处理
    if (flag_R && time_data_R > 15)
    {  // 当时间>15ms时接收完成
        if (arr_data_R[0] == 'l' && arr_data_R[1] == 'a' && arr_data_R[2] == 'n')
        {  // 格式化输出形式在他前面加\r\n
            sprintf((char*) arr_data_T, "\r\nYES!\r\n");
            HAL_UART_Transmit(&huart1, arr_data_T, sizeof(arr_data_T), 50);
            led_show(8, 1);
        }
        else
        {
            sprintf((char*) arr_data_T, "\r\nERROR!\r\n");
            HAL_UART_Transmit(&huart1, arr_data_T, sizeof(arr_data_T), 50);
            led_show(8, 0);
        }
        // memset函数快速初始化
        memset(arr_data_T, 0, 20);
        memset(arr_data_R, 0, 20);
        // 标志位和字符串置0
        counter_data_R = 0;
        flag_R = 0;
    }
}  // 模拟电压测量
void adc_vol(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_Start(&hadc2);
    vol_37 = 3.3 * HAL_ADC_GetValue(&hadc1) / 4095;
    vol_38 = 3.3 * HAL_ADC_GetValue(&hadc2) / 4095;
}  // 输入捕获，注意：不要设置ARR的值
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM2)
    {
        TIM2->CNT = 0;
        fre1 = 1000000 / (TIM2->CCR1 + 1);
    }
    if (htim->Instance == TIM3)
    {
        TIM3->CNT = 0;
        fre2 = 1000000 / (TIM3->CCR1 + 1);
    }
}
void data_proc(void)
{
    if (counter > 5 && led8.led_last_statu == 0)
    {
        led8.led_flag_en = 1;
        led8.time_counter = 0;
    }
    // 核心逻辑： 三元表达式实现当counter>5时led_last_statu=1 否则为0
    // 而且要写在后面，这样上一个if语句才会先执行，
    led8.led_last_statu = (counter > 5) ? 1 : 0;

    if (led8.time_counter > 2000 && led8.led_flag_en)
    {
        if (led8.time_counter <= 7000)
        {  // 更优化的算发，使用取余实现led闪烁
           //  三元表达式实现闪烁，时间差每100ms闪烁一次
            led8.led_current_statu = (led8.time_counter % 200 + 1 <= 100) ? 1 : 0;

            // if (led8.time_counter % 200 + 1 <= 100)

            //     led8.led_current_statu = 1;
            // else
            //     led8.led_current_statu = 0;
        }
        else
        {  // 当大于7秒时，闪烁结束，重置所有参数，注意此else是否定的小于于7秒的条件
            led8.led_flag_en = 0;
            led8.led_current_statu = 0;
        }
    }
    // led流水灯
    if (time % 1000 + 1 <= 500)
        led_show(1, 1);
    else
        led_show(1, 0);
}
void main_proc(void)
{
    key_scan();
    lcd_show();
    data_proc();
    U_data_R_proc();
    adc_vol();
    led_show(8, led8.led_current_statu);
}
