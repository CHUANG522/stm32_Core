#ifndef _FUN_H
#define _FUN_H

#include "Header.h"

void lcd_show(void);
void led_show(uint8_t led, uint8_t mode);
void data_proc(void);
void main_proc(void);
void key_scan(void);
void adc_vlo(void);
void time_show(void);

#endif
