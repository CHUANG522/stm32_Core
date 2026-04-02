#ifndef _FUN_H
#define _FUN_H

#include "Header.h"

void led_show(uint8_t led, uint8_t mode);
void lcd_show(void);
void data_proc(void);
void main_proc(void);
void key_scan(void);
extern uint8_t data_R; // 供给main文件使用

void data_R_proc(void);
// 以下供给滴答定时器使用
extern uint32_t time_usart1;
extern uint32_t time_CCR, time_led;
extern uint8_t led_flag;

extern uint8_t counter_led, false_flag_3;
#endif
