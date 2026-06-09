#include "RTC.h"


/* 内部BCD转换函数 */
static uint8_t RTC_DecToBcd(uint8_t val);
static uint8_t RTC_BcdToDec(uint8_t val);

/* 内部初始化参数结构体（GD32固件库风格） */
static rtc_parameter_struct s_rtc_initpara;

/**
 * @brief  十进制转BCD
 */
static uint8_t RTC_DecToBcd(uint8_t val)
{
    uint8_t bcdhigh = 0;
    while (val >= 10) {
        bcdhigh++;
        val -= 10;
    }
    return (bcdhigh << 4) | val;
}

/**
 * @brief  BCD转十进制
 */
static uint8_t RTC_BcdToDec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}

/**
 * @brief  判断闰年（完整规则）
 */
bool RTC_IsLeapYear(uint16_t year)
{
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        return true;
    }
    return false;
}

/**
 * @brief  获取某年某月的天数
 */
uint8_t RTC_GetDaysInMonth(uint16_t year, uint8_t month)
{
    const uint8_t days_tab[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (month == 2 && RTC_IsLeapYear(year)) {
        return 29;
    }
    return days_tab[month - 1];
}

/**
 * @brief  计算星期几（蔡勒公式简化版，1901-2099）
 * @retval 1=周一, 2=周二 ... 7=周日
 */
uint8_t RTC_CalcWeekday(uint16_t year, uint8_t month, uint8_t day)
{
    uint8_t yearh = year / 100;
    uint8_t yearl = year % 100;
    
    // 月修正表：1月=0, 2月=3, 3月=3, 4月=6...
    const uint8_t month_tab[12] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};
    
    // 21世纪年份+100
    if (yearh > 19) {
        yearl += 100;
    }
    
    uint16_t temp = yearl + (yearl / 4);
    temp = temp % 7;
    temp = temp + day + month_tab[month - 1];
    
    // 闰年且1月或2月，减1
    if (RTC_IsLeapYear(year) && month < 3) {
        temp--;
    }
    
    temp %= 7;
    if (temp == 0) {
        temp = 7;
    }
    return (uint8_t)temp;
}


static uint8_t RTC_ValidateDateTime(const RTC_DateTimeTypeDef *dt)
{
    if (dt == NULL) 
	{
		return 1;
	}
    
    if (dt->year > RTC_YEAR_MAX) 
	{
		
		return 1;
	}
    if (dt->month < RTC_MONTH_MIN || dt->month > RTC_MONTH_MAX) 
	{	
		return 1;
	}
    if (dt->date < RTC_DATE_MIN || dt->date > RTC_DATE_MAX) 
	{
		return 1;
	}
    if (dt->hour > RTC_HOUR_MAX) 
	{
		return 1;
	}
    if (dt->min > RTC_MIN_MAX) 
	{
		return 1;
	}
    if (dt->sec > RTC_SEC_MAX) 
	{
		return 1;
	}
    if (dt->week < RTC_WEEK_MIN || dt->week > RTC_WEEK_MAX) 
	{
		return 1;
	}
    
    // 检查日期是否超过当月最大天数
    uint16_t full_year = 2000 + dt->year;
    if (dt->date > RTC_GetDaysInMonth(full_year, dt->month)) 
	{
        return 1;
    }
    
    return 0;
}


uint8_t RTC_Init(void)
{

    uint16_t prescaler_s = 0;
    uint16_t prescaler_a = 0;
    uint16_t bkpflag = 0;
    uint8_t clk_src = 0;
    
    /* 1. 使能电源接口时钟，备份域写使能 */
    rcu_periph_clock_enable(RCU_PMU);
    pmu_backup_write_enable();
    
    /* 2. 读取备份寄存器标记 */
    bkpflag = RTC_BKP0;
    
    /* 3. 尝试启动LSE */
    rcu_osci_on(RCU_LXTAL);
    
    if (rcu_osci_stab_wait(RCU_LXTAL) == SUCCESS) {
        /* LSE成功 */
		
		
		
        rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

        prescaler_s = LSE_PRESCALER_SYNC;
        prescaler_a = LSE_PRESCALER_ASYNC;
        RTC_BKP0 = RTC_BKP_FLAG_LSE;
        clk_src = RTC_CLKSRC_LSE;
		
    } else {
        /* LSE失败，尝试LSI */
		
		
        rcu_osci_on(RCU_IRC32K);
        if (rcu_osci_stab_wait(RCU_IRC32K) == ERROR) {
			
            return 1;  // 时钟源全部失败
        }
        rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);
        prescaler_s = LSI_PRESCALER_SYNC;
        prescaler_a = LSI_PRESCALER_ASYNC;
        RTC_BKP0 = RTC_BKP_FLAG_LSI;
        clk_src = RTC_CLKSRC_LSI;
    }
    
    /* 4. 使能RTC时钟并等待同步 */
    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();
    
    /* 5. 判断是否需要初始化时间（第一次上电或标记异常） */
    uint8_t rtc_src_flag = GET_BITS(RCU_BDCTL, 8, 9);
    
    if ((bkpflag != RTC_BKP_FLAG_LSE && bkpflag != RTC_BKP_FLAG_LSI) || 
        (rtc_src_flag == 0)) 
	{
        LED2_ON();
        /* 需要初始化默认时间 */
        s_rtc_initpara.factor_asyn = prescaler_a;
        s_rtc_initpara.factor_syn = prescaler_s;
        s_rtc_initpara.display_format = RTC_24HOUR;
		
        
        s_rtc_initpara.year = RTC_DecToBcd(RTC_DEFAULT_YEAR);
        s_rtc_initpara.month = RTC_DecToBcd(RTC_DEFAULT_MONTH);
        s_rtc_initpara.date = RTC_DecToBcd(RTC_DEFAULT_DATE);
        s_rtc_initpara.day_of_week = RTC_DEFAULT_WEEK;
        s_rtc_initpara.hour = RTC_DecToBcd(RTC_DEFAULT_HOUR);
        s_rtc_initpara.minute = RTC_DecToBcd(RTC_DEFAULT_MIN);
        s_rtc_initpara.second = RTC_DecToBcd(RTC_DEFAULT_SEC);
        s_rtc_initpara.am_pm = RTC_AM;
        if (rtc_init(&s_rtc_initpara) == ERROR) {
			
            return 2;
        }
    }
    
    (void)clk_src;  // 消除未使用警告
	
    return 0;
}

bool RTC_IsInitialized(void)
{
    uint16_t bkp = RTC_BKP0;
    return (bkp == RTC_BKP_FLAG_LSE || bkp == RTC_BKP_FLAG_LSI);
}

/**
 * @brief  获取当前时钟源
 */
RTC_ClkSourceTypeDef RTC_GetClockSource(void)
{
    uint16_t bkp = RTC_BKP0;
    if (bkp == RTC_BKP_FLAG_LSE) return RTC_CLKSRC_LSE;
    if (bkp == RTC_BKP_FLAG_LSI) return RTC_CLKSRC_LSI;
    return RTC_CLKSRC_NONE;
}

uint8_t RTC_SetDateTime(const RTC_DateTimeTypeDef *dt)
{
    if (RTC_ValidateDateTime(dt) != 0) {
		RS485_Printf("RTC_ValidateDateTime(dt) != 0");
        return 1;  // 参数非法
    }
    
    /* 填充初始化结构体 */
	s_rtc_initpara.factor_asyn = LSE_PRESCALER_ASYNC;
    s_rtc_initpara.factor_syn = LSE_PRESCALER_SYNC;
    s_rtc_initpara.year = RTC_DecToBcd(dt->year);
    s_rtc_initpara.month = RTC_DecToBcd(dt->month);
    s_rtc_initpara.date = RTC_DecToBcd(dt->date);
    s_rtc_initpara.day_of_week = dt->week;
    s_rtc_initpara.hour = RTC_DecToBcd(dt->hour);
    s_rtc_initpara.minute = RTC_DecToBcd(dt->min);
    s_rtc_initpara.second = RTC_DecToBcd(dt->sec);
    s_rtc_initpara.am_pm = (dt->am_pm & 0x01);
    s_rtc_initpara.display_format = RTC_24HOUR;
    
    /* 重新初始化RTC（GD32库函数会同时更新时间和日期） */
    if (rtc_init(&s_rtc_initpara) == ERROR) {
        return 2;
    }
    
    return 0;
}

void RTC_GetDateTime(RTC_DateTimeTypeDef *dt)
{
    if (dt == NULL) return;
    
    /* 从硬件读取到内部结构体 */
    rtc_current_time_get(&s_rtc_initpara);
    
    /* BCD转十进制 */
    dt->year = RTC_BcdToDec(s_rtc_initpara.year);
    dt->month = RTC_BcdToDec(s_rtc_initpara.month);
    dt->date = RTC_BcdToDec(s_rtc_initpara.date);
    dt->week = s_rtc_initpara.day_of_week;
    dt->hour = RTC_BcdToDec(s_rtc_initpara.hour);
    dt->min = RTC_BcdToDec(s_rtc_initpara.minute);
    dt->sec = RTC_BcdToDec(s_rtc_initpara.second);
    dt->am_pm = s_rtc_initpara.am_pm & 0x01;
}

void RTC_SetAlarmA(uint8_t week, uint8_t hour, uint8_t min, uint8_t sec)
{
    rtc_alarm_struct alarm;
    
    rtc_alarm_disable(RTC_ALARM0);
    
    alarm.alarm_mask = 0;
    alarm.weekday_or_date = RTC_ALARM_WEEKDAY_SELECTED;
    alarm.alarm_day = RTC_DecToBcd(week);
    alarm.alarm_hour = RTC_DecToBcd(hour);
    alarm.alarm_minute = RTC_DecToBcd(min);
    alarm.alarm_second = RTC_DecToBcd(sec);
    alarm.am_pm = RTC_AM;
    
    rtc_alarm_config(RTC_ALARM0, &alarm);
    
    rtc_interrupt_enable(RTC_INT_ALARM0);
    rtc_alarm_enable(RTC_ALARM0);
    rtc_flag_clear(RTC_FLAG_ALRM0);
    
    exti_flag_clear(EXTI_17);
    exti_init(EXTI_17, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    nvic_irq_enable(RTC_Alarm_IRQn, 1, 2);
}

void RTC_SetWakeupTimer(uint8_t wksel, uint16_t cnt)
{
    rtc_wakeup_disable();
    rtc_wakeup_clock_set(wksel);
    rtc_wakeup_timer_set(cnt);
    rtc_flag_clear(RTC_FLAG_WT);
    rtc_interrupt_enable(RTC_INT_WAKEUP);
    rtc_wakeup_enable();
    
    exti_flag_clear(EXTI_22);
    exti_init(EXTI_22, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    nvic_irq_enable(RTC_WKUP_IRQn, 2, 2);
}

void RTC_Alarm_IRQHandler(void)
{
    if (rtc_flag_get(RTC_FLAG_ALRM0) != RESET) {
        rtc_flag_clear(RTC_FLAG_ALRM0);
        /* 用户在此处添加业务逻辑 */
    }
    exti_flag_clear(EXTI_17);
}


void RTC_WKUP_IRQHandler(void)
{
    if (rtc_flag_get(RTC_FLAG_WT) != RESET) {
        rtc_flag_clear(RTC_FLAG_WT);
        /* 用户在此处添加业务逻辑 */
    }
    exti_flag_clear(EXTI_22);
}






