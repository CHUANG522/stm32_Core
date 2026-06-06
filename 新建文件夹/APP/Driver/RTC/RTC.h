#ifndef __RTC_H
#define __RTC_H

#include "HeaderFiles.h"

#define RTC_BKP_DR0             0       // 备份寄存器0
#define RTC_BKP_FLAG_LSE          0x7050  // 标记：使用LSE
#define RTC_BKP_FLAG_LSI          0x7051  // 标记：使用LSI
#define RTC_BKP_FLAG_NONE         0x0000  // 标记：未初始化

#define RTC_DEFAULT_YEAR          25      // 2025
#define RTC_DEFAULT_MONTH         6
#define RTC_DEFAULT_DATE          13
#define RTC_DEFAULT_HOUR          11
#define RTC_DEFAULT_MIN           30
#define RTC_DEFAULT_SEC           0
#define RTC_DEFAULT_WEEK          5       // 周五

/* 时钟源频率定义 */
#define LSE_FREQ_HZ               32768   // 外部晶振 32.768kHz
#define LSI_FREQ_HZ               32000   // 内部RC 32kHz（GD32典型值，STM32为32.768k）

/* 分频值计算（目标：1Hz） */
#define LSE_PRESCALER_SYNC        255     // 256分频 (0xFF)
#define LSE_PRESCALER_ASYNC       127     // 128分频 (0x7F)
#define LSI_PRESCALER_SYNC        319     // 320分频 (0x13F)
#define LSI_PRESCALER_ASYNC       99      // 100分频 (0x63)

/* 校验范围 */
#define RTC_YEAR_MIN              0       // 2000
#define RTC_YEAR_MAX              99      // 2099
#define RTC_MONTH_MIN             1
#define RTC_MONTH_MAX             12
#define RTC_DATE_MIN              1
#define RTC_DATE_MAX              31
#define RTC_HOUR_MIN              0
#define RTC_HOUR_MAX              23
#define RTC_MIN_MIN               0
#define RTC_MIN_MAX               59
#define RTC_SEC_MIN               0
#define RTC_SEC_MAX               59
#define RTC_WEEK_MIN              1
#define RTC_WEEK_MAX              7

/*============================================================
 *  数据结构
 *============================================================*/
typedef struct {
    uint8_t year;       // 0~99 (2000~2099)
    uint8_t month;      // 1~12
    uint8_t date;       // 1~31
    uint8_t week;       // 1=周一, 7=周日
    uint8_t hour;       // 0~23
    uint8_t min;        // 0~59
    uint8_t sec;        // 0~59
    uint8_t am_pm;      // 0=24H/AM, 1=PM (12H模式用)
} RTC_DateTimeTypeDef;

typedef enum {
    RTC_CLKSRC_LSE = 0, // 外部低速晶振（推荐，精度高）
    RTC_CLKSRC_LSI,     // 内部RC振荡器（备用）
    RTC_CLKSRC_NONE     // 未配置
} RTC_ClkSourceTypeDef;

/*============================================================
 *  API 声明
 *============================================================*/

// 初始化
uint8_t RTC_Init(void);
bool RTC_IsInitialized(void);

// 时间设置（统一接口，避免覆盖）
uint8_t RTC_SetDateTime(const RTC_DateTimeTypeDef *dt);

// 时间读取
void RTC_GetDateTime(RTC_DateTimeTypeDef *dt);

// 时钟源查询
RTC_ClkSourceTypeDef RTC_GetClockSource(void);

// 工具函数：星期计算（蔡勒公式，1901-2099）
uint8_t RTC_CalcWeekday(uint16_t year, uint8_t month, uint8_t day);

// 工具函数：闰年判断
bool RTC_IsLeapYear(uint16_t year);

// 工具函数：月份天数
uint8_t RTC_GetDaysInMonth(uint16_t year, uint8_t month);

// 闹钟与唤醒（基础接口）
void RTC_SetAlarmA(uint8_t week, uint8_t hour, uint8_t min, uint8_t sec);
void RTC_SetWakeupTimer(uint8_t wksel, uint16_t cnt);

// 中断服务（声明为弱定义，方便用户重写）
void RTC_Alarm_IRQHandler(void);
void RTC_WKUP_IRQHandler(void);

#endif /* __RTC_DRIVER_H */
