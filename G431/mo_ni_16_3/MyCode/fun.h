#ifndef _FUN_H
#define _FUN_H

#include "Header.h"
// 函数
void main_pors(void);
void led_show(uint8_t led, uint8_t mode);
void key_scan(void);
void lcd_show(void);
void calc_threshold(void);
float adc_read(uint8_t Rx);
void date_proc(void);
// 全局变量
extern uint8_t B1_state, B1_last, B2_state, B2_last, B3_state, B3_last, B4_state, B4_last;
extern int counter;
extern uint8_t string[20];
extern uint8_t led_falg;
extern uint16_t free1;
extern uint8_t lcd_page;
extern uint8_t rate;
extern float vol_value;

#endif
