#ifndef _FUN_H
#define _FUN_H

#include "Head.h"
extern char     string[20];
extern uint16_t ADC_Value1,ADC_Value2;
extern double   Vol_Value1,Vol_Value2;
extern uint8_t  Rec_temp ;
extern void     LCD_Show (void);
extern double   ADC_Vol (uint8_t ADC);
extern uint8_t  Rec_arr[20];
extern uint8_t  Send_arr[20];
extern uint8_t  Rec_Flag;
extern uint8_t  counter;
void Uart_Date_Rec(void);
#endif
