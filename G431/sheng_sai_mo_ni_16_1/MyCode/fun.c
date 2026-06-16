#include "fun.h"

uint8_t lcd_page;
char data_R;
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

    B1_last = B1_statu;
    B2_last = B2_statu;
    B3_last = B3_statu;
    B4_last = B4_statu;
}

char string[20];
uint32_t lcd_time;

void lcd_show(void)
{
    if (uwTick - lcd_time < 10)
        return;
    lcd_time = uwTick;
    if (lcd_page == 0)
    {
        sprintf(string, "  eeprom_data %d    ", eeprom_data);
        LCD_DisplayStringLine(Line0, (uint8_t *)string);

        sprintf(string, "        Lock       ");
        LCD_DisplayStringLine(Line1, (uint8_t *)string);

        sprintf(string, "      Pass Word    ");
        LCD_DisplayStringLine(Line3, (uint8_t *)string);

        sprintf(string, "        0 1 *      ");
        LCD_DisplayStringLine(Line4, (uint8_t *)string);
    }
    if (lcd_page == 1)
    {

        sprintf(string, "        Set        ");
        LCD_DisplayStringLine(Line1, (uint8_t *)string);

        sprintf(string, "       Change      ");
        LCD_DisplayStringLine(Line3, (uint8_t *)string);

        sprintf(string, "       1 0 *       ");
        LCD_DisplayStringLine(Line4, (uint8_t *)string);
    }
}
uint8_t data_R_counter;
uint8_t R_flag; // 1为接收到数据
uint8_t temp_R[20];
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        R_flag = 1;
        temp_R[data_R_counter++] = data_R;
        TIM4->CNT = 0;
        led_show(6, 1);
        HAL_UART_Transmit(&huart1, (uint8_t *)&data_R, 1, 50);
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&data_R, 1);
    }
}
uint8_t temp_T[20];
void data_R_proc(void)
{
    if (R_flag && TIM4->CNT > 1500)
    {
        led_show(8, 1);
        if (temp_R[0] == 'H' && temp_R[1] == 'A' && temp_R[2] == 'L')
        {

            sprintf((char *)temp_T, "YES!完美 \r\n");
            HAL_UART_Transmit(&huart1, temp_T, sizeof(temp_T), 50);
        }
        else
        {
            sprintf((char *)temp_T, "Error \r\n");
            HAL_UART_Transmit(&huart1, temp_T, sizeof(temp_T), 50);
        }
        memset(temp_R, 0, 20);
        R_flag = data_R_counter = 0;
    }
}

void data_proc(void)
{
}

void main_proc(void)
{
    key_scan();
    lcd_show();
    data_R_proc();
    led_show(1, 1);
}
