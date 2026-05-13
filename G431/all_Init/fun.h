#ifndef _FUN_H
#define _FUN_H

#include "Header.h"
// 函数
void led_show(uint8_t led, uint8_t mode);
void key_scan(void);
void lcd_show(void);
void data_proc(void);
void main_proc(void);
void data_R_proc(void);
void U_data_R_proc(void);
void adc_vol(void);

// 全局变量
extern uint32_t time_B1, time_data_R;  // fun 滴答定时器
extern uint8_t data_R;                 // main和fun共用
extern uint8_t eeprom_data;            // main ，fun ，i2c
#endif
