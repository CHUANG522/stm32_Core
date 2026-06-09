#include "Command.h"
#include "Protocol.h"
#include "USART1.h"
#include "RTC_func.h"
#include "RTC.h"
#include "ADc.h"
#include "Function.h"
#include "PT100.h"

extern uint16_t g_device_id;
extern uint8_t s_baudrate_code;
extern volatile float g_ch0_ratio,g_ch1_ratio;
extern volatile uint8_t  g_auto_report_enabled;
extern volatile uint16_t g_report_interval_tick;
extern volatile uint16_t g_report_tick_counter;
extern volatile float g_ch0_threshold;
extern volatile float g_ch1_threshold;
extern volatile float g_ch2_threshold;

static void Cmd_SendOkAck(ProtocolFrame_t *frame)
{
    uint8_t ok = 0xFF;
    Protocol_SendAck(frame->device_id, frame->cmd_word, &ok, 1);
}

void Protocol_SendErrorFrame(uint16_t device_id)
{
    uint8_t empty = 0x00;
    Protocol_SendFrame(device_id, 0xFF, CMD_ERROR_GENERAL, &empty, 0);
}

void Cmd_ProcessFrame(ProtocolFrame_t *frame)
{
    if (frame == NULL) return;

    if (frame->frame_type != FRAME_TYPE_CMD &&
        frame->frame_type != FRAME_TYPE_ACK &&
        frame->frame_type != FRAME_TYPE_HEART)
    {
        printf("Err: Unknown frame_type 0x%02X\r\n", frame->frame_type);
        Protocol_SendErrorFrame(frame->device_id);
        return;
    }

    switch (frame->cmd_word)
    {
    case CMD_DEVICE_REBOOT:       Cmd_DeviceReboot(frame);       break;
    case CMD_FACTORY_RESET:       Cmd_FactoryReset(frame);       break;
    case CMD_QUERY_DEVICE_INFO:   Cmd_QueryDeviceInfo(frame);    break;
    case CMD_QUERY_FW_VERSION:    Cmd_QueryFwVersion(frame);     break;
    case CMD_SET_TIME:            Cmd_SetTime(frame);            break;
    case CMD_QUERY_TIME:          Cmd_QueryTime(frame);          break;
    case CMD_SET_DEVICE_ID:       Cmd_SetDeviceID(frame);        break;
    case CMD_SET_BAUDRATE:        Cmd_SetBaudrate(frame);        break;
    case CMD_QUERY_DEVICE_ID:     Cmd_QueryDeviceID(frame);      break;
    case CMD_QUERY_BAUDRATE:      Cmd_QueryBaudrate(frame);      break;
    case CMD_QUERY_CH0:           Cmd_QueryCh0(frame);           break;
    case CMD_QUERY_CH1:           Cmd_QueryCh1(frame);           break;
    case CMD_QUERY_CH2_PT100:     Cmd_QueryCh2Pt100(frame);      break;
    case CMD_SET_CH0_RATIO:       Cmd_SetCh0Ratio(frame);        break;
    case CMD_SET_CH1_RATIO:       Cmd_SetCh1Ratio(frame);        break;
    case CMD_SET_REPORT_INTERVAL: Cmd_SetReportInterval(frame);  break;
    case CMD_SET_DAC_OUTPUT:      Cmd_SetDacOutput(frame);       break;
    case CMD_START_AUTO_REPORT:   Cmd_StartAutoReport(frame);    break;
    case CMD_STOP_AUTO_REPORT:    Cmd_StopAutoReport(frame);     break;
    case CMD_ENTER_SLEEP:         Cmd_EnterSleep(frame);         break;
    case CMD_READ_THRESHOLDS:     Cmd_ReadThresholds(frame);     break;
    case CMD_READ_CH0_THRESHOLD:  Cmd_ReadCh0Threshold(frame);   break;
    case CMD_READ_CH1_THRESHOLD:  Cmd_ReadCh1Threshold(frame);   break;
    case CMD_READ_CH2_THRESHOLD:  Cmd_ReadCh2Threshold(frame);   break;
    case CMD_WRITE_CH0_THRESHOLD: Cmd_WriteCh0Threshold(frame);  break;
    case CMD_WRITE_CH1_THRESHOLD: Cmd_WriteCh1Threshold(frame);  break;
    case CMD_WRITE_CH2_THRESHOLD: Cmd_WriteCh2Threshold(frame);  break;
    case CMD_UPGRADE_REQUEST:     Cmd_UpgradeRequest(frame);     break;
    case CMD_PREPARE_FIRMWARE:    Cmd_PrepareFirmware(frame);    break;
    case CMD_EXECUTE_UPGRADE:     Cmd_ExecuteUpgrade(frame);     break;
    case CMD_SET_ALARM_MODE:      Cmd_SetAlarmMode(frame);       break;
    case CMD_QUERY_ALARM_RECORD:  Cmd_QueryAlarmRecord(frame);   break;
    case CMD_CLEAR_ALARM_RECORD:  Cmd_ClearAlarmRecord(frame);   break;
    case CMD_HEARTBEAT:           Cmd_Heartbeat(frame);          break;
    case CMD_BROADCAST_FIND:      Cmd_BroadcastFind(frame);      break;

    default:
        printf("Err: Unknown cmd_word 0x%04X\r\n", frame->cmd_word);
        Protocol_SendErrorFrame(frame->device_id);
        break;
    }
}

#define RCU_MODIFY_4(__delay)   do{                                     \
    volatile uint32_t i, reg;           \
    if(0 != __delay){                   \
        for(i=0; i<__delay; i++){       \
        }                               \
        reg = RCU_CFG0;                 \
        reg &= ~(RCU_CFG0_AHBPSC);      \
        reg |= RCU_AHB_CKSYS_DIV2;      \
        RCU_CFG0 = reg;                 \
        for(i=0; i<__delay; i++){       \
        }                               \
        reg = RCU_CFG0;                 \
        reg &= ~(RCU_CFG0_AHBPSC);      \
        reg |= RCU_AHB_CKSYS_DIV4;      \
        RCU_CFG0 = reg;                 \
        for(i=0; i<__delay; i++){       \
        }                               \
        reg = RCU_CFG0;                 \
        reg &= ~(RCU_CFG0_AHBPSC);      \
        reg |= RCU_AHB_CKSYS_DIV8;      \
        RCU_CFG0 = reg;                 \
        for(i=0; i<__delay; i++){       \
        }                               \
        reg = RCU_CFG0;                 \
        reg &= ~(RCU_CFG0_AHBPSC);      \
        reg |= RCU_AHB_CKSYS_DIV16;     \
        RCU_CFG0 = reg;                 \
        for(i=0; i<__delay; i++){       \
        }                               \
    }                                   \
}while(0)

static void _soft_delay_(uint32_t time)
{
    __IO uint32_t i;
    for (i = 0; i < time * 10; i++)
    {
    }
}

static void rcu_config(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;
    rcu_deinit();
    FMC_WS = 0x57U;
    RCU_CTL |= RCU_CTL_HXTALEN;
    do {
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_HXTALSTB);
    } while ((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));
    if (0U == (RCU_CTL & RCU_CTL_HXTALSTB)) {
        while (0U == (RCU_CTL & RCU_CTL_HXTALSTB)) {}
    }
    RCU_APB1EN |= RCU_APB1EN_PMUEN;
    PMU_CTL |= PMU_CTL_LDOVS;
    RCU_CFG0 &= ~(RCU_CFG0_AHBPSC | RCU_CFG0_APB1PSC | RCU_CFG0_APB2PSC);
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV4;
    RCU_PLL = (25U | (480U << 6U) | (((2U >> 1U) - 1U) << 16U) |
               (RCU_PLLSRC_HXTAL) | (10U << 24U));
    RCU_CTL |= RCU_CTL_PLLEN;
    while (0U == (RCU_CTL & RCU_CTL_PLLSTB)) {}
    PMU_CTL |= PMU_CTL_HDEN;
    while (0U == (PMU_CS & PMU_CS_HDRF)) {}
    PMU_CTL |= PMU_CTL_HDS;
    while (0U == (PMU_CS & PMU_CS_HDSRF)) {}
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLLP;
    while (0U == (RCU_CFG0 & RCU_SCSS_PLLP)) {}
    rcu_timer_clock_prescaler_config(RCU_TIMER_PSC_MUL4);
}

static void SendAutoReportFrame(uint16_t device_id)
{
    RTC_DateTimeTypeDef dt;
    RTC_GetDateTime(&dt);
    if (dt.year  > 99 || dt.month < 1 || dt.month > 12 ||
        dt.date  < 1  || dt.date  > 31 ||
        dt.hour  > 23 || dt.min   > 59 || dt.sec > 59)
        return;

    DateTime_t in;
    in.year  = (uint16_t)dt.year + 2000;
    in.month = dt.month;
    in.day   = dt.date;
    in.hour  = dt.hour;
    in.min   = dt.min;
    in.sec   = dt.sec;

    uint32_t utc_ts = DateTimeToUnix(&in, +8);

    uint8_t report[12];
    report[0] = (utc_ts >> 24) & 0xFF;
    report[1] = (utc_ts >> 16) & 0xFF;
    report[2] = (utc_ts >>  8) & 0xFF;
    report[3] = (utc_ts >>  0) & 0xFF;
    FloatToBeBytes(Query_CH0(), &report[4]);
    FloatToBeBytes(Query_CH1(), &report[8]);
    Protocol_SendAck(device_id, CMD_START_AUTO_REPORT, report, 12);
}

void Cmd_AutoReportTick(uint16_t device_id)
{
    float ch0, ch1, ch2;
    if (!g_auto_report_enabled)
        return;

    g_report_tick_counter++;
    if (g_report_tick_counter >= g_report_interval_tick)
    {
        g_report_tick_counter = 0;
        ch0 = Query_CH0();
        ch1 = Query_CH1();
        ch2 = Query_CH2();
        SendAutoReportFrame(device_id);
        if (ch0 > g_ch0_threshold) {
            Alarm_CheckAndReport(device_id, 0, ch0, g_ch0_threshold);
        }
        if (ch1 > g_ch1_threshold) {
            Alarm_CheckAndReport(device_id, 1, ch1, g_ch1_threshold);
        }
        if (ch2 > g_ch2_threshold) {
            Alarm_CheckAndReport(device_id, 2, ch2, g_ch2_threshold);
        }
    }
}

void Cmd_DeviceReboot(ProtocolFrame_t *frame)
{
    Cmd_SendOkAck(frame);
    mcu_software_reset();
}

void Cmd_FactoryReset(ProtocolFrame_t *frame)
{
    Cmd_SendOkAck(frame);
}

void Cmd_QueryDeviceInfo(ProtocolFrame_t *frame)
{
    Cmd_SendOkAck(frame);
}

void Cmd_QueryFwVersion(ProtocolFrame_t *frame)
{
    LED1_ON();
    uint8_t ver[4];
    spi_flash_read_version(0x0000, ver);
    Protocol_SendAck(frame->device_id, CMD_QUERY_FW_VERSION, ver, 4);
}

void Cmd_SetTime(ProtocolFrame_t *frame)
{
    if (frame == NULL || frame->data_len < 4)
    {
        Protocol_SendError(frame ? frame->device_id : 0xFFFF);
        return;
    }
    uint32_t utc = ((uint32_t)frame->data[0] << 24) |
                   ((uint32_t)frame->data[1] << 16) |
                   ((uint32_t)frame->data[2] << 8)  |
                   ((uint32_t)frame->data[3]);
    DateTime_t out;
    UnixToDateTime(utc, +8, &out);
    if (out.year < 2000 || out.year > 2099 ||
        out.month < 1  || out.month > 12 ||
        out.day   < 1  || out.day   > 31 ||
        out.hour  > 23 || out.min   > 59 || out.sec > 59)
    {
        Protocol_SendError(frame->device_id);
        return;
    }
    Up_RTC_Time((uint8_t)(out.year - 2000), out.month, out.day,
                out.hour, out.min, out.sec);
    Cmd_SendOkAck(frame);
}

void Cmd_QueryTime(ProtocolFrame_t *frame)
{
    RTC_DateTimeTypeDef dt;
    RTC_GetDateTime(&dt);
    if (dt.year  > 99 || dt.month < 1 || dt.month > 12 ||
        dt.date  < 1  || dt.date  > 31 ||
        dt.hour  > 23 || dt.min   > 59 || dt.sec > 59)
    {
        Protocol_SendError(frame->device_id);
        return;
    }
    DateTime_t in;
    in.year  = (uint16_t)dt.year + 2000;
    in.month = dt.month;
    in.day   = dt.date;
    in.hour  = dt.hour;
    in.min   = dt.min;
    in.sec   = dt.sec;

    uint32_t utc_ts = DateTimeToUnix(&in, +8);
    uint8_t utc[4];
    utc[0] = (utc_ts >> 24) & 0xFF;
    utc[1] = (utc_ts >> 16) & 0xFF;
    utc[2] = (utc_ts >>  8) & 0xFF;
    utc[3] = (utc_ts >>  0) & 0xFF;
    Protocol_SendAck(frame->device_id, CMD_QUERY_TIME, utc, 4);
}

extern uint8_t device_id[2];
void Cmd_SetDeviceID(ProtocolFrame_t *frame)
{
    if (frame->data_len >= 2)
    {
        uint8_t new_id[2];
        new_id[0] = frame->data[0];
        new_id[1] = frame->data[1];
        g_device_id = new_id[0] << 8 | new_id[1];
		
		device_id[0] = new_id[0];
		device_id[1] = new_id[1];
		
        uint8_t ver[4], f;
        spi_flash_buffer_read(ver, 0x0000, 4);
        spi_flash_buffer_read(&f, 0x001B, 1);
		
        spi_flash_sector_erase(0x0000);
        delay_1ms(50);
		
        spi_flash_buffer_write(ver, 0x0000, 4);
        delay_1ms(5);
        spi_flash_buffer_write(new_id, 0x0004, 2);
        delay_1ms(5);
        spi_flash_buffer_write(&f, 0x001B, 1);
        delay_1ms(5);
    }
    uint8_t ok = 0xFF;
    Protocol_SendAck(g_device_id, frame->cmd_word, &ok, 1);
}

void Cmd_SetBaudrate(ProtocolFrame_t *frame)
{
    if (frame->data_len >= 1)
    {
        uint8_t code = frame->data[0];
        if (code != 0x11 && code != 0x12 && code != 0x13 && code != 0x14) {
            Protocol_SendError(frame->device_id);
            return;
        }
        uint8_t ver[4], id[2], f,value[2];
        spi_flash_buffer_read(ver, 0x0000, 4);
        spi_flash_buffer_read(id, 0x0004, 2);
        spi_flash_buffer_read(&f, 0x001B, 1);
		spi_flash_buffer_read(value, 0x001C, 2);
        spi_flash_sector_erase(0x0000);
        delay_1ms(50);
        spi_flash_buffer_write(ver, 0x0000, 4);
        delay_1ms(5);
        spi_flash_buffer_write(id, 0x0004, 2);
        delay_1ms(5);
        spi_flash_buffer_write(&code, 0x0006, 1);
        delay_1ms(5);
        spi_flash_buffer_write(&f,    0x001B, 1);
        delay_1ms(5);
        spi_flash_buffer_write(value,    0x001C, 2);
        delay_1ms(5);		
        s_baudrate_code = code;
        Cmd_SendOkAck(frame);
        delay_1ms(100);
        RS485_ReInit(Baudrate_CodeToValue(code));
        return;
    }
    Cmd_SendOkAck(frame);
}
void Cmd_QueryDeviceID(ProtocolFrame_t *frame)
{
    uint8_t id[2];
    spi_flash_buffer_read(id, 0x0004, 2);
    Protocol_SendAck(frame->device_id, CMD_QUERY_DEVICE_ID, id, 2);
}

void Cmd_QueryBaudrate(ProtocolFrame_t *frame)
{
    spi_flash_buffer_read(&s_baudrate_code, 0x0006, 1);
    Protocol_SendAck(frame->device_id, CMD_QUERY_BAUDRATE, &s_baudrate_code, 1);
}

void Cmd_QueryCh0(ProtocolFrame_t *frame)
{
    uint8_t f[4];
    FloatToBeBytes(Query_CH0(), f);
    Protocol_SendAck(frame->device_id, CMD_QUERY_CH0, f, 4);
}
void Cmd_QueryCh1(ProtocolFrame_t *frame)
{
    uint8_t f[4];
    FloatToBeBytes(Query_CH1(), f);
    Protocol_SendAck(frame->device_id, CMD_QUERY_CH1, f, 4);
}
void Cmd_QueryCh2Pt100(ProtocolFrame_t *frame)
{
    uint8_t f[4];
    FloatToBeBytes(Query_CH2(), f);
    Protocol_SendAck(frame->device_id, CMD_QUERY_CH2_PT100, f, 4);
}
void Cmd_SetCh0Ratio(ProtocolFrame_t *frame)
{
    if (frame == NULL || frame->data_len < 4)
    {
        Protocol_SendError(frame ? frame->device_id : 0xFFFF);
        return;
    }
    g_ch0_ratio = BeBytesToFloat(frame->data);
    uint8_t ver[4], id[2], baud, f,value[2];
    spi_flash_buffer_read(ver,  0x0000, 4);
    spi_flash_buffer_read(id,   0x0004, 2);
    spi_flash_buffer_read(&baud, 0x0006, 1);
    spi_flash_buffer_read(&f,    0x001B, 1);
	spi_flash_buffer_read(value, 0x001C, 2);
	
    spi_flash_sector_erase(0x0000);
    delay_1ms(50);
    spi_flash_buffer_write(ver,  0x0000, 4);  delay_1ms(5);
    spi_flash_buffer_write(id,   0x0004, 2);  delay_1ms(5);
    spi_flash_buffer_write(&baud, 0x0006, 1);  delay_1ms(5);
    spi_flash_buffer_write(&f, 0x001B, 1);    delay_1ms(5);
	spi_flash_buffer_write(value,    0x001C, 2);delay_1ms(5);
    spi_flash_buffer_write(frame->data, 0x0007, 4);
    delay_1ms(5);
    Cmd_SendOkAck(frame);
}
void Cmd_SetCh1Ratio(ProtocolFrame_t *frame)
{
    if (frame == NULL || frame->data_len < 4)
    {
        Protocol_SendError(frame ? frame->device_id : 0xFFFF);
        return;
    }
    g_ch1_ratio = BeBytesToFloat(frame->data);
    uint8_t ver[4], id[2], baud, ch0[4], f,value[2];
    spi_flash_buffer_read(ver,  0x0000, 4);
    spi_flash_buffer_read(id,   0x0004, 2);
    spi_flash_buffer_read(&baud, 0x0006, 1);
    spi_flash_buffer_read(ch0,  0x0007, 4);
    spi_flash_buffer_read(&f,   0x001B, 1);
	spi_flash_buffer_read(value, 0x001C, 2);
	
    spi_flash_sector_erase(0x0000);
    delay_1ms(50);
    spi_flash_buffer_write(ver,  0x0000, 4);  delay_1ms(5);
    spi_flash_buffer_write(id,   0x0004, 2);  delay_1ms(5);
    spi_flash_buffer_write(&baud, 0x0006, 1);  delay_1ms(5);
    spi_flash_buffer_write(ch0,  0x0007, 4);  delay_1ms(5);
    spi_flash_buffer_write(&f,   0x001B, 1);  delay_1ms(5);
	spi_flash_buffer_write(value,    0x001C, 2);delay_1ms(5);
    spi_flash_buffer_write(frame->data, 0x000B, 4);
    delay_1ms(5);
    Cmd_SendOkAck(frame);
}
void Cmd_SetReportInterval(ProtocolFrame_t *frame)
{
    if (frame == NULL || frame->data_len < 1)
    {
        Protocol_SendError(frame ? frame->device_id : 0xFFFF);
        return;
    }
    uint8_t code = frame->data[0];
    uint16_t tick;
    switch (code)
    {
    case 0x01: tick = 100; break;
    case 0x02: tick = 300; break;
    case 0x03: tick = 500; break;
    default:
        Protocol_SendError(frame->device_id);
        return;
    }
    g_report_interval_tick = tick;
    Cmd_SendOkAck(frame);
}

void Cmd_SetDacOutput(ProtocolFrame_t *frame)
{
    if (frame == NULL || frame->data_len < 2)
    {
        Protocol_SendError(frame ? frame->device_id : 0xFFFF);
        return;
    }
    uint16_t dac_value = ((uint16_t)frame->data[0] << 8) | frame->data[1];
    if (dac_value > 4095)
    {
        Protocol_SendError(frame->device_id);
        return;
    }
    DAC_Set_Value(dac_value);

    uint8_t ver[4], id[2], baud, ch0[4], ch1[4], th0[4], th1[4], th2[4], f;
    spi_flash_buffer_read(ver,   0x0000, 4);
    spi_flash_buffer_read(id,    0x0004, 2);
    spi_flash_buffer_read(&baud, 0x0006, 1);
    spi_flash_buffer_read(ch0,   0x0007, 4);
    spi_flash_buffer_read(ch1,   0x000B, 4);
    spi_flash_buffer_read(th0,   0x000F, 4);
    spi_flash_buffer_read(th1,   0x0013, 4);
    spi_flash_buffer_read(th2,   0x0017, 4);
    spi_flash_buffer_read(&f,    0x001B, 1);

    uint8_t dac_be[2];
    dac_be[0] = (dac_value >> 8) & 0xFF;
    dac_be[1] = dac_value & 0xFF;

    spi_flash_sector_erase(0x0000);
    delay_1ms(50);
    spi_flash_buffer_write(ver,   0x0000, 4);  delay_1ms(5);
    spi_flash_buffer_write(id,    0x0004, 2);  delay_1ms(5);
    spi_flash_buffer_write(&baud, 0x0006, 1);  delay_1ms(5);
    spi_flash_buffer_write(ch0,   0x0007, 4);  delay_1ms(5);
    spi_flash_buffer_write(ch1,   0x000B, 4);  delay_1ms(5);
    spi_flash_buffer_write(th0,   0x000F, 4);  delay_1ms(5);
    spi_flash_buffer_write(th1,   0x0013, 4);  delay_1ms(5);
    spi_flash_buffer_write(th2,   0x0017, 4);  delay_1ms(5);
    spi_flash_buffer_write(&f,    0x001B, 1);  delay_1ms(5);
    spi_flash_buffer_write(dac_be, 0x001C, 2); delay_1ms(5);

    Cmd_SendOkAck(frame);
}

void Cmd_StartAutoReport(ProtocolFrame_t *frame)
{
    if (g_auto_report_enabled)
    {
        SendAutoReportFrame(frame->device_id);
        return;
    }
    g_auto_report_enabled = 1;
    g_report_tick_counter = 0;
    SendAutoReportFrame(frame->device_id);
}

void Cmd_StopAutoReport(ProtocolFrame_t *frame)
{
    g_auto_report_enabled = 0;
    g_report_tick_counter = 0;
    Cmd_SendOkAck(frame);
}

void Cmd_EnterSleep(ProtocolFrame_t *frame)
{
	LED1_OFF();
    OLED_Clear();
	
    Cmd_SendOkAck(frame);
    delay_1ms(100);
    RTC_DateTimeTypeDef dt;
    RTC_GetDateTime(&dt);
    uint8_t alarm_sec = dt.sec + 10;
    uint8_t alarm_min = dt.min;
    uint8_t alarm_hour = dt.hour;
    uint8_t alarm_week = dt.week;
    if (alarm_sec >= 60) {
        alarm_sec -= 60;
        alarm_min++;
        if (alarm_min >= 60) {
            alarm_min -= 60;
            alarm_hour++;
            if (alarm_hour >= 24)
                alarm_hour = 0;
        }
    }
    RTC_SetAlarmA(alarm_week, alarm_hour, alarm_min, alarm_sec);
    usart_interrupt_disable(RS485_USART, USART_INT_RBNE);
    pmu_to_deepsleepmode(PMU_LDO_LOWPOWER, PMU_LOWDRIVER_ENABLE, WFI_CMD);
    SCB->SCR &= ~(uint32_t)SCB_SCR_SLEEPDEEP_Msk;
    rcu_config();
    rtc_alarm_disable(RTC_ALARM0);
    rtc_flag_clear(RTC_FLAG_ALRM0);
    exti_interrupt_flag_clear(EXTI_17);
    exti_flag_clear(EXTI_17);
    rcu_periph_clock_enable(RS485_USART_CLK);
    usart_disable(RS485_USART);
    delay_1ms(1);
    usart_enable(RS485_USART);
    nvic_irq_enable(RS485_IRQn, 3, 1);
    usart_interrupt_enable(RS485_USART, USART_INT_RBNE);
    fwdgt_counter_reload();
    RS485_SendData((uint8_t*)"instrument wakeup\r\n", 19);
}

static void Threshold_WriteToFlash(void)
{
    uint8_t ver[4], id[2], baud, ch0[4], ch1[4], f,value[2];
    spi_flash_buffer_read(ver,       0x0000, 4);
    spi_flash_buffer_read(id,        0x0004, 2);
    spi_flash_buffer_read(&baud,     0x0006, 1);
    spi_flash_buffer_read(ch0,       0x0007, 4);
    spi_flash_buffer_read(ch1,       0x000B, 4);
    spi_flash_buffer_read(&f,        0x001B, 1);
	spi_flash_buffer_read(value, 0x001C, 2);

    uint8_t th0[4], th1[4], th2[4];
    FloatToBeBytes(g_ch0_threshold, th0);
    FloatToBeBytes(g_ch1_threshold, th1);
    FloatToBeBytes(g_ch2_threshold, th2);

    spi_flash_sector_erase(0x0000);
    delay_1ms(50);
    spi_flash_buffer_write(ver,       0x0000, 4);  delay_1ms(5);
    spi_flash_buffer_write(id,        0x0004, 2);  delay_1ms(5);
    spi_flash_buffer_write(&baud,     0x0006, 1);  delay_1ms(5);
    spi_flash_buffer_write(ch0,       0x0007, 4);  delay_1ms(5);
    spi_flash_buffer_write(ch1,       0x000B, 4);  delay_1ms(5);
    spi_flash_buffer_write(th0,       0x000F, 4);  delay_1ms(5);
    spi_flash_buffer_write(th1,       0x0013, 4);  delay_1ms(5);
    spi_flash_buffer_write(th2,       0x0017, 4);  delay_1ms(5);
    spi_flash_buffer_write(&f,        0x001B, 1);  delay_1ms(5);
	spi_flash_buffer_write(value,    0x001C, 2);   delay_1ms(5);
}

void Cmd_ReadThresholds(ProtocolFrame_t *frame)
{
    uint8_t th[8];
    FloatToBeBytes(g_ch0_threshold, &th[0]);
    FloatToBeBytes(g_ch1_threshold, &th[4]);
    Protocol_SendAck(frame->device_id, CMD_READ_THRESHOLDS, th, 8);
}

void Cmd_ReadCh0Threshold(ProtocolFrame_t *frame)
{
    uint8_t th[4];
    FloatToBeBytes(g_ch0_threshold, th);
    Protocol_SendAck(frame->device_id, CMD_READ_CH0_THRESHOLD, th, 4);
}

void Cmd_ReadCh1Threshold(ProtocolFrame_t *frame)
{
    uint8_t th[4];
    FloatToBeBytes(g_ch1_threshold, th);
    Protocol_SendAck(frame->device_id, CMD_READ_CH1_THRESHOLD, th, 4);
}

void Cmd_ReadCh2Threshold(ProtocolFrame_t *frame)
{
    uint8_t th[4];
    FloatToBeBytes(g_ch2_threshold, th);
    Protocol_SendAck(frame->device_id, CMD_READ_CH2_THRESHOLD, th, 4);
}

void Cmd_WriteCh0Threshold(ProtocolFrame_t *frame)
{
    if (frame == NULL || frame->data_len < 4)
    {
        Protocol_SendError(frame ? frame->device_id : 0xFFFF);
        return;
    }
    g_ch0_threshold = BeBytesToFloat(frame->data);
    Threshold_WriteToFlash();
    Cmd_SendOkAck(frame);
}

void Cmd_WriteCh1Threshold(ProtocolFrame_t *frame)
{
    if (frame == NULL || frame->data_len < 4)
    {
        Protocol_SendError(frame ? frame->device_id : 0xFFFF);
        return;
    }
    g_ch1_threshold = BeBytesToFloat(frame->data);
    Threshold_WriteToFlash();
    Cmd_SendOkAck(frame);
}

void Cmd_WriteCh2Threshold(ProtocolFrame_t *frame)
{
    if (frame == NULL || frame->data_len < 4)
    {
        Protocol_SendError(frame ? frame->device_id : 0xFFFF);
        return;
    }
    g_ch2_threshold = BeBytesToFloat(frame->data);
    Threshold_WriteToFlash();
    Cmd_SendOkAck(frame);
}

void Cmd_UpgradeRequest(ProtocolFrame_t *frame)
{
    uint8_t ack = 0xFF;
    Protocol_SendAck(frame->device_id, CMD_UPGRADE_REQUEST, &ack, 1);
    APP_HandleUpgradeRequest();
}

void Cmd_PrepareFirmware(ProtocolFrame_t *frame)
{
    uint8_t ack = 0xFF;
    Protocol_SendAck(frame->device_id, CMD_PREPARE_FIRMWARE, &ack, 1);
}

void Cmd_ExecuteUpgrade(ProtocolFrame_t *frame)
{
    uint8_t ack = 0xFF;
    Protocol_SendAck(frame->device_id, CMD_EXECUTE_UPGRADE, &ack, 1);
}

#define ALARM_FLASH_ADDR        0x1000
#define ALARM_MODE_OFFSET       0
#define ALARM_COUNT_OFFSET      1
#define ALARM_RECORD_OFFSET     5
#define ALARM_RECORD_SIZE       16
#define ALARM_MAX_RECORDS       10

typedef struct {
    uint32_t utc;
    uint8_t  channel;
    uint8_t  reserved[3];
    float    threshold;
    float    actual;
} AlarmRecord_t;

static void Alarm_ReadRecord(uint32_t addr, AlarmRecord_t *rec)
{
    uint8_t buf[16];
    spi_flash_buffer_read(buf, addr, 16);
    rec->utc       = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
                     ((uint32_t)buf[2] << 8)  | buf[3];
    rec->channel   = buf[4];
    rec->threshold = BeBytesToFloat(&buf[8]);
    rec->actual    = BeBytesToFloat(&buf[12]);
}

static void Alarm_WriteRecord(uint32_t addr, const AlarmRecord_t *rec)
{
    uint8_t buf[16];
    buf[0] = (rec->utc >> 24) & 0xFF;
    buf[1] = (rec->utc >> 16) & 0xFF;
    buf[2] = (rec->utc >> 8)  & 0xFF;
    buf[3] = rec->utc & 0xFF;
    buf[4] = rec->channel;
    buf[5] = buf[6] = buf[7] = 0;
    FloatToBeBytes(rec->threshold, &buf[8]);
    FloatToBeBytes(rec->actual,    &buf[12]);
    spi_flash_buffer_write(buf, addr, 16);
    delay_1ms(5);
}

static void Alarm_RecordToString(const AlarmRecord_t *rec, char *buf, uint16_t buf_size)
{
    DateTime_t dt;
    UnixToDateTime(rec->utc, +8, &dt);
    const char *ch_str = (rec->channel == 0) ? "CH0" :
                         (rec->channel == 1) ? "CH1" : "CH2";
    snprintf(buf, buf_size,
             "%04d-%02d-%02d %02d:%02d:%02d | %s | %.2f | %.2f\r\n",
             dt.year, dt.month, dt.day,
             dt.hour, dt.min, dt.sec,
             ch_str, (double)rec->threshold, (double)rec->actual);
}

static void Alarm_AddRecord(uint8_t channel, float actual, float threshold)
{
    AlarmRecord_t records[ALARM_MAX_RECORDS];
    uint8_t mode = 0, count = 0, i;
    spi_flash_buffer_read(&mode,  ALARM_FLASH_ADDR + ALARM_MODE_OFFSET,  1);
    spi_flash_buffer_read(&count, ALARM_FLASH_ADDR + ALARM_COUNT_OFFSET, 1);
    if (count > ALARM_MAX_RECORDS) count = 0;

    for (i = 0; i < count; i++) {
        Alarm_ReadRecord(ALARM_FLASH_ADDR + ALARM_RECORD_OFFSET + i * ALARM_RECORD_SIZE,
                         &records[i]);
    }

    AlarmRecord_t new_rec;
    RTC_DateTimeTypeDef rdt;
    RTC_GetDateTime(&rdt);
    DateTime_t in;
    in.year  = (uint16_t)rdt.year + 2000;
    in.month = rdt.month;
    in.day   = rdt.date;
    in.hour  = rdt.hour;
    in.min   = rdt.min;
    in.sec   = rdt.sec;
    new_rec.utc       = DateTimeToUnix(&in, +8);
    new_rec.channel   = channel;
    new_rec.threshold = threshold;
    new_rec.actual    = actual;

    for (i = (count < ALARM_MAX_RECORDS ? count : ALARM_MAX_RECORDS - 1); i > 0; i--) {
        records[i] = records[i - 1];
    }
    records[0] = new_rec;
    if (count < ALARM_MAX_RECORDS) count++;

    spi_flash_sector_erase(ALARM_FLASH_ADDR);
    delay_1ms(50);
    spi_flash_buffer_write(&mode,  ALARM_FLASH_ADDR + ALARM_MODE_OFFSET,  1);
    delay_1ms(5);
    spi_flash_buffer_write(&count, ALARM_FLASH_ADDR + ALARM_COUNT_OFFSET, 1);
    delay_1ms(5);
    for (i = 0; i < count; i++) {
        Alarm_WriteRecord(ALARM_FLASH_ADDR + ALARM_RECORD_OFFSET + i * ALARM_RECORD_SIZE,
                          &records[i]);
    }
}

void Alarm_CheckAndReport(uint16_t device_id, uint8_t channel, float actual, float threshold)
{
    (void)device_id;
    if (actual <= threshold) return;

    Alarm_AddRecord(channel, actual, threshold);

    uint8_t mode = 0;
    spi_flash_buffer_read(&mode, ALARM_FLASH_ADDR + ALARM_MODE_OFFSET, 1);
    if (mode == 0x01) {
        AlarmRecord_t rec;
        Alarm_ReadRecord(ALARM_FLASH_ADDR + ALARM_RECORD_OFFSET, &rec);
        char buf[64];
        Alarm_RecordToString(&rec, buf, sizeof(buf));
        RS485_SendData((uint8_t *)buf, strlen(buf));
    }
}

void Cmd_SetAlarmMode(ProtocolFrame_t *frame)
{
    if (frame == NULL || frame->data_len < 1)
    {
        Protocol_SendError(frame ? frame->device_id : 0xFFFF);
        return;
    }
    uint8_t mode = frame->data[0];
    if (mode != 0x01 && mode != 0x02)
    {
        Protocol_SendError(frame->device_id);
        return;
    }
    uint8_t count = 0;
    spi_flash_buffer_read(&count, ALARM_FLASH_ADDR + ALARM_COUNT_OFFSET, 1);
    if (count > ALARM_MAX_RECORDS) count = 0;

    AlarmRecord_t records[ALARM_MAX_RECORDS];
    uint8_t i;
    for (i = 0; i < count; i++) {
        Alarm_ReadRecord(ALARM_FLASH_ADDR + ALARM_RECORD_OFFSET + i * ALARM_RECORD_SIZE,
                         &records[i]);
    }
    spi_flash_sector_erase(ALARM_FLASH_ADDR);
    delay_1ms(50);
    spi_flash_buffer_write(&mode,  ALARM_FLASH_ADDR + ALARM_MODE_OFFSET,  1);
    delay_1ms(5);
    spi_flash_buffer_write(&count, ALARM_FLASH_ADDR + ALARM_COUNT_OFFSET, 1);
    delay_1ms(5);
    for (i = 0; i < count; i++) {
        Alarm_WriteRecord(ALARM_FLASH_ADDR + ALARM_RECORD_OFFSET + i * ALARM_RECORD_SIZE,
                          &records[i]);
    }
    Cmd_SendOkAck(frame);
}

void Cmd_QueryAlarmRecord(ProtocolFrame_t *frame)
{
    (void)frame;
    uint8_t count = 0;
    spi_flash_buffer_read(&count, ALARM_FLASH_ADDR + ALARM_COUNT_OFFSET, 1);
    if (count == 0 || count > ALARM_MAX_RECORDS)
    {
        RS485_SendData((uint8_t *)"empty", 5);
        return;
    }
    uint8_t i;
    for (i = 0; i < count; i++)
    {
        AlarmRecord_t rec;
        Alarm_ReadRecord(ALARM_FLASH_ADDR + ALARM_RECORD_OFFSET + i * ALARM_RECORD_SIZE,
                         &rec);
        char buf[64];
        Alarm_RecordToString(&rec, buf, sizeof(buf));
        RS485_SendData((uint8_t *)buf, strlen(buf));
        delay_1ms(10);
    }
}

void Cmd_ClearAlarmRecord(ProtocolFrame_t *frame)
{
    uint8_t mode = 0;
    spi_flash_buffer_read(&mode, ALARM_FLASH_ADDR + ALARM_MODE_OFFSET, 1);
    spi_flash_sector_erase(ALARM_FLASH_ADDR);
    delay_1ms(50);
    spi_flash_buffer_write(&mode, ALARM_FLASH_ADDR + ALARM_MODE_OFFSET, 1);
    delay_1ms(5);
    uint8_t zero = 0;
    spi_flash_buffer_write(&zero, ALARM_FLASH_ADDR + ALARM_COUNT_OFFSET, 1);
    delay_1ms(5);
    Cmd_SendOkAck(frame);
}

void Cmd_Heartbeat(ProtocolFrame_t *frame)
{
    (void)frame;
}

void Cmd_BroadcastFind(ProtocolFrame_t *frame)
{
    Protocol_SendHeartbeat(g_device_id);
}