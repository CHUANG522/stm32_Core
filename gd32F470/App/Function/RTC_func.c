#include "RTC_func.h"
RTC_DateTimeTypeDef dt;
uint8_t tick_RTC_Sec = 0;
#include <stdint.h>
#include <stdbool.h>
/*================ 辅助：闰年判断 =================*/
static bool is_leap_year(uint16_t year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

/* 每月天数，索引 0 = 1月 */
static const uint8_t days_in_month[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};


bool tick_1s(void)
{
    RTC_DateTimeTypeDef tmp;
    RTC_GetDateTime(&tmp);  // 直接读硬件
    
    if (tick_RTC_Sec != tmp.sec)
    {
        tick_RTC_Sec = tmp.sec;
        dt = tmp;  // 同步更新全局 dt
        return 1;
    }
    return 0;
}


void RTC_TimeRefresh(void)
{
	RTC_GetDateTime(&dt);
}

void rtc_now_handler(const char* args)
{
	RTC_TimeRefresh();
	
	rtc_now_printf("Current Time:");
	
}

void rtc_now_printf(char* args)
{
	RTC_TimeRefresh();
	RS485_Printf("%s20%02d-%02d-%02d ",args,dt.year,dt.month,dt.date);
	RS485_Printf("%02d:%02d:%02d\r\n",dt.hour,dt.min,dt.sec);
	
}

void Up_RTC_Time(uint8_t years, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec)
{
    uint16_t full_year = 2000 + years;
    dt.week = RTC_CalcWeekday(full_year, month, date);
	
	dt.year = years;
	dt.month = month;
	dt.date = date;
	dt.hour = hour;
	dt.min = min;
	dt.sec = sec;
	
    if(RTC_SetDateTime(&dt))
	{
		RS485_Printf("RTC Config error\r\n");
        return ;  // 日期设置失败
    }   
}

void rtc_config_setdata(const char* args)
{
	int year, month, day, hour, min, sec;
    
    if (sscanf(args, "%d-%d-%d %d:%d:%d", 
               &year, &month, &day, &hour, &min, &sec) != 6) 
    {
        if (sscanf(args, "%d-%d-%d-%d-%d-%d", 
                   &year, &month, &day, &hour, &min, &sec) != 6) 
        {
            RS485_Printf("Invalid RTC Config format\r\n");
            return;
        }
    }
    
    // 验证并转换年份为2位表示
    if (year < 2000 || year > 2099) {
        RS485_Printf("Year must be between 2000-2099\r\n");
        return;
    }
    uint8_t years = (uint8_t)(year - 2000);
    
    // 验证各参数范围
    if (month < 1 || month > 12 || day < 1 || day > 31 || 
        hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 59) 
    {
        RS485_Printf("Invalid date/time values\r\n");
        return;
    }
    
    // 设置RTC时间
	Up_RTC_Time(years, (uint8_t)month, (uint8_t)day, (uint8_t)hour, (uint8_t)min, (uint8_t)sec);
	
	rtc_now_printf("Time:");	
}

void rtc_config_handler(const char* args)
{
	RS485_SendData((const uint8_t *)"Input Datetime\r\n",strlen("Input Datetime\r\n"));	
}

uint32_t DateTimeToUnix(const DateTime_t* dt, int8_t tz_offset_hours)
{
    uint32_t total_days = 0;
    uint16_t y;

    /* 1. 计算 1970 年到目标年份前一年的总天数 */
    for (y = 1970; y < dt->year; y++) {
        total_days += is_leap_year(y) ? 366 : 365;
    }

    /* 2. 加上当年到目标月份之前的天数 */
    for (uint8_t m = 1; m < dt->month; m++) {
        total_days += days_in_month[m - 1];
        if (m == 2 && is_leap_year(dt->year)) {
            total_days++;  /* 闰年 2 月加 1 天 */
        }
    }

    /* 3. 加上当月已过天数（day - 1） */
    total_days += dt->day - 1;

    /* 4. 计算当天秒数 */
    uint32_t total_seconds = total_days * 86400UL
                         + dt->hour * 3600UL
                         + dt->min  * 60UL
                         + dt->sec;

    /* 5. 减去时区偏移，得到 UTC 时间戳 */
    total_seconds -= (int32_t)tz_offset_hours * 3600UL;

    return total_seconds;
}


void UnixToDateTime(uint32_t unix_ts, int8_t tz_offset_hours, DateTime_t* out)
{
    /* 1. 加上时区偏移，得到本地总秒数 */
    int32_t local_seconds = (int32_t)unix_ts + (int32_t)tz_offset_hours * 3600;
    if (local_seconds < 0) local_seconds = 0;

    uint32_t sec_total = (uint32_t)local_seconds;

    /* 2. 拆分天数和当天秒数 */
    uint32_t days_since_1970 = sec_total / 86400UL;
    uint32_t sec_of_day      = sec_total % 86400UL;

    /* 3. 计算时分秒 */
    out->sec  = sec_of_day % 60;
    out->min  = (sec_of_day / 60) % 60;
    out->hour = sec_of_day / 3600;

    /* 4. 计算年份 */
    uint16_t year = 1970;
    while (1) {
        uint32_t year_days = is_leap_year(year) ? 366 : 365;
        if (days_since_1970 < year_days) break;
        days_since_1970 -= year_days;
        year++;
    }
    out->year = year;

    /* 5. 计算月份和日期 */
    uint8_t month = 1;
    while (month <= 12) {
        uint8_t dim = days_in_month[month - 1];
        if (month == 2 && is_leap_year(year)) dim++;
        if (days_since_1970 < dim) break;
        days_since_1970 -= dim;
        month++;
    }
    out->month = month;
    out->day   = (uint8_t)days_since_1970 + 1;
}

/*================ 便捷包装函数（UTC 零时区）================*/


uint32_t DateTimeToUnixUTC(const DateTime_t* dt)
{
    return DateTimeToUnix(dt, 0);
}

void UnixToDateTimeUTC(uint32_t unix_ts, DateTime_t* out)
{
    UnixToDateTime(unix_ts, 0, out);
}
