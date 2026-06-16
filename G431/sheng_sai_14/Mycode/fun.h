#ifndef  _FUN_H
#define  _FUN_H

#include "Header.h"

//函数
void led_show(uint8_t led,uint8_t mode);
void key_scan(void);
void lcd_show(void);
void data_proc(void);
void main_proc(void);
void ADC_vlo(void);

//全局变量
extern uint32_t time_key;
extern uint32_t time_lcd;
extern uint32_t time_fre;
extern uint8_t time_fre_counter,low_high,high_low,low_high_counter;
extern uint8_t B2_flag;
extern uint32_t time_B2;





#endif
