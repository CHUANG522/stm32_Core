#include "Header.h"

void led_show(uint8_t led, uint8_t mode)
{
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);

  if (mode == 1)
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_RESET);
  else
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 << (led - 1), GPIO_PIN_SET);

  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
}
// 按键部分

uint8_t B1_state = 1, B1_last = 1, B2_state = 1, B2_last = 1, B3_state = 1,
        B3_last = 1, B4_state = 1, B4_last = 1;
uint8_t led_falg = 1;
uint8_t lcd_page = 0;
uint8_t rate;
uint16_t free1;

float vol_value; // 存放电压值
uint8_t rate_max;
uint8_t rate_min;
uint8_t con_counter;
uint8_t high = 100, low = 60;
uint8_t high_last = 100, low_last = 60;
uint8_t high_low;
uint8_t falg_in_out;

void key_scan(void)
{

  B1_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
  B2_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1);
  B3_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
  B4_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
  // B1
  if (!B1_state && B1_last)
  {

    if (++lcd_page > 2)
    {
      lcd_page = 0;
      falg_in_out = 0;
      high_last = high;
      low_last = low;
    }
  }
  // led_falg ^= 1; // 异或运算符，两个二进制位相同则为 0，不同则为 1。

  // B2
  if (!B2_state & B2_last)
  {
    if (lcd_page == 2)
    {

      high_low = (high_low + 1) % 2;
    }
  }
  // B3
  if (!B3_state && B3_last)
  {
    if (lcd_page == 2)
    {
      // 切换修改状态
      falg_in_out = (falg_in_out + 1) % 2;

      if (falg_in_out == 1)
      {
        // 进入修改状态：备份当前值，用于恢复
        high_last = high;
        low_last = low;
      }
    }
  }

  // B4
  if (!B4_state & B4_last)
  {
    if (lcd_page == 0)
    {
      con_counter = 0;
    }
  }

  B1_last = B1_state;
  B2_last = B2_state;
  B3_last = B3_state;
  B4_last = B4_state;
}
// lcd显示部分
uint8_t string[20];
void lcd_show(void)
{
  switch (lcd_page)
  {
  case 0:

    sprintf((char *)string, "      HEART   ");
    LCD_DisplayStringLine(Line1, string);

    sprintf((char *)string, "     Rate :%d  ", rate);
    LCD_DisplayStringLine(Line3, string);

    sprintf((char *)string, "      Con :    ");
    LCD_DisplayStringLine(Line4, string);
    break;
  case 1:

    sprintf((char *)string, "       RECORD      ");
    LCD_DisplayStringLine(Line1, string);

    sprintf((char *)string, "       Max:%d      ", rate_max);
    LCD_DisplayStringLine(Line3, string);

    sprintf((char *)string, "       Min:%d      ", rate_min);
    LCD_DisplayStringLine(Line4, string);
    break;
  case 2:

    sprintf((char *)string, "        PARA       ");
    LCD_DisplayStringLine(Line1, string);

    sprintf((char *)string, "     High:%d       ", high);
    LCD_DisplayStringLine(Line3, string);

    sprintf((char *)string, "      Low:%d       ", low);
    LCD_DisplayStringLine(Line4, string);
    break;
  }
}
// 读取ADC的电压，报警有关部分

float adc_read(uint8_t Rx)
{ // 如果形参ADC_HandleTypeDef *hadc 不能使用就这样写
  float value;
  if (Rx == 37)
  {
    HAL_ADC_Start(&hadc2);
    value = 3.3 * HAL_ADC_GetValue(&hadc2) / 4096;
  }
  //  else
  // {
  //    HAL_ADC_Start(&hadc1);
  //    adc_value = 3.3 * HAL_ADC_GetValue(&hadc1) / 4096;
  // }
  return value;
}
// 心率有关部分以及变量

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM16)
  {
    uint16_t captuer_value = TIM16->CCR1 + 1; // 此处加1更准确一点
    TIM16->CNT = 0;
    free1 = 1000000 / captuer_value;
  }
}
//高低数据处理函数
void calc_threshold(void)
{
  vol_value = adc_read(37);
  if (falg_in_out)
  {

    if (high_low == 0)
    {
      if (vol_value < 1)
        high = 60;
      else if (vol_value > 3)
        high = 150;
      else
        high = 45 * vol_value + 15;
    }
    if (high_low == 1)
    {
      if (vol_value < 1)
        low = 60;
      else if (vol_value > 3)
        low = 150;
      else
        low = 45 * vol_value + 15;
    }
  }
}
// 数据处理函数

void date_proc(void)
{
  // 心率处理部分
  if (free1 < 1000)
    rate = 30;
  else if (free1 > 2000)
    rate = 200;
  else
    rate = 0.17 * free1 - 140;
  // 报警数值部分
  calc_threshold();

  // 心率部分2
  if (rate > rate_max)
    rate_max = rate;
  if (rate < rate_min)
    rate_min = rate;
}

// main循环函数
void main_pors(void)
{
  lcd_show();
  key_scan();
  date_proc();
  led_show(8, led_falg);
}
