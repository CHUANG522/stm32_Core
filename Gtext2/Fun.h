#ifndef _Fun_h
#define _Fun_h

#include "Head.h"
void LED_Show(uint8_t LED,uint8_t mode);
void Key_Scan(void);
void LCD_Show(void);

//全局变量
extern uint8_t  Key_B1;
extern uint8_t  Key_B1_Last ;
extern uint8_t  Key_B2;
extern uint8_t  Key_B2_Last ;
extern uint8_t  Key_B3;
extern uint8_t  Key_B3_Last ;

extern char     string[20];
extern int      counter;
extern uint8_t  LED_Mode;
extern uint8_t  High_Line ;
extern uint32_t fre,capture_value ; 
//宏定义
#define Sys_fre (80000000) //系统时钟
#define Psc     (80-1)     //预分频
#define High_Show_Num (5) //高亮显示行数

//
#endif
