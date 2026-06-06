#include "RTC_func.h"

RTC_DateTimeTypeDef dt;
uint8_t tick_RTC_Sec = 0;

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
	

	
	SysFlag.Usart_Set_flag=0;
	rtc_now_printf("Time:");
	
	
}

void rtc_config_handler(const char* args)
{

	SysFlag.Usart_Set_flag = 1;
	RS485_SendData((const uint8_t *)"Input Datetime\r\n",strlen("Input Datetime\r\n"));	
}


// 闰年判断函数
static int is_leap_year(uint16_t year) 
{
    if (year % 400 == 0) return 1;
    if (year % 100 == 0) return 0;
    return (year % 4 == 0);
}

// 将系统时间转换为Unix时间戳
uint32_t convert_to_unix(void) 
{
    const uint32_t base_ts = 946684800;

    const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    uint32_t total_days = 0;

    for (uint16_t y = 2000; y < (2000 + dt.year); y++) {
        total_days += is_leap_year(y) ? 366 : 365;
    }

    for (uint8_t m = 1; m < dt.month; m++) {
        total_days += days_in_month[m-1];
        if (m == 2 && is_leap_year(2000 + dt.year)) {
            total_days++;
        }
    }

    total_days += dt.date - 1;

    uint32_t total_seconds = total_days * 86400UL + 
                             dt.hour * 3600UL + 
                             dt.min * 60UL + 
                             dt.sec - 8*3600UL;
    
    return base_ts + total_seconds;
}

char* Hide_Unix(float data)
{
	 static char hex_buffer[17];
	
	uint32_t unix_ts = convert_to_unix();
    
    uint16_t integer_part = (uint16_t)data; 
    float fractional = data - integer_part;
    uint16_t fractional_part = (uint16_t)(fractional * 65536.0f + 0.5f); 
    
    uint32_t voltage = ((uint32_t)integer_part << 16) | fractional_part;
    
    sprintf(hex_buffer, "%08lX%08lX", (unsigned long)unix_ts, (unsigned long)voltage);
	
	RS485_Printf( "%08lX%08lX", (unsigned long)unix_ts, (unsigned long)voltage);
	
	return hex_buffer;
}




