#ifndef _FUN_H
#define _FUN_H

#include "Header.h"
void led_show(uint8_t led, uint8_t mode);

void key_scan(void);
void lcd_show(void);
void data_proc(void);
void main_proc(void);
extern char  data_R;
void data_R_proc(void);
#endif
