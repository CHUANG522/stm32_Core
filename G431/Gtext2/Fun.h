#ifndef _Fun_h
#define _Fun_h

#include "Head.h"
void lcd_show(void);
void key_scan(void);
void led_show(uint8_t led, uint8_t mode);
void led_SHOW(void);
void data_proc(void);
void main_proc(void);
float adc_get(void);

#endif
