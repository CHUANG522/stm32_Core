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
extern uint32_t time;
// 结构体的声明要在.h文件中
typedef struct
{
    uint8_t led_flag_en;     // 是否开启闪烁1为开
    uint8_t led_last_statu;  //
    uint8_t led_current_statu;
    uint32_t time_counter;
} led_shan;  // 结构体实现LED闪烁功能，记录当前时间，当前状态，闪烁频率及时长，上次状态
extern led_shan led8;

#endif
