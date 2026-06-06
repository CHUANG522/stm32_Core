#ifndef __RTC_FUNC_H
#define __RTC_FUNC_H

#include "HeaderFiles.h"



extern uint8_t tick_RTC_Sec;
bool tick_1s(void);
void RTC_TimeRefresh(void);
void rtc_now_printf(char* args);
void rtc_now_handler(const char* args);
void Up_RTC_Time(uint8_t years, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec);
void rtc_config_handler(const char* args);
void rtc_config_setdata(const char* args);


static int is_leap_year(uint16_t year);
uint32_t convert_to_unix(void);
char* Hide_Unix(float data);

#endif

