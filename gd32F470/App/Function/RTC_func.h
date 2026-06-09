#ifndef __RTC_FUNC_H
#define __RTC_FUNC_H

#include "HeaderFiles.h"
/*================ 日期时间结构体 =================*/
typedef struct {
    uint16_t year;  /* 绝对年份，如 2026 */
    uint8_t  month; /* 1 ~ 12 */
    uint8_t  day;   /* 1 ~ 31 */
    uint8_t  hour;  /* 0 ~ 23 */
    uint8_t  min;   /* 0 ~ 59 */
    uint8_t  sec;   /* 0 ~ 59 */
} DateTime_t;


extern uint8_t tick_RTC_Sec;
bool tick_1s(void);
void RTC_TimeRefresh(void);
void rtc_now_printf(char* args);
void rtc_now_handler(const char* args);
void Up_RTC_Time(uint8_t years, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec);
void rtc_config_handler(const char* args);
void rtc_config_setdata(const char* args);


static bool is_leap_year(uint16_t year);


uint32_t DateTimeToUnix(const DateTime_t* dt, int8_t tz_offset_hours);
void UnixToDateTime(uint32_t unix_ts, int8_t tz_offset_hours, DateTime_t* out);

uint32_t DateTimeToUnixUTC(const DateTime_t* dt);
void UnixToDateTimeUTC(uint32_t unix_ts, DateTime_t* out);











#endif

