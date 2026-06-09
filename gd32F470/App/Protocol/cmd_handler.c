//#include "cmd_handler.h"


///* ===================== 外部依赖接口（用户需实现） ===================== */
///* 若以下函数未定义，编译会报错，请在你的 Driver/Function 层实现 */

///* 时间 */
////extern uint32_t RTC_GetUTC(void);
////extern void     RTC_SetUTC(uint32_t utc);

///* 数据采集 */
////extern float ADC_GetCH0_Value(void);    /* 返回：原始采样 × 当前变比 */
////extern float ADC_GetCH1_Value(void);    /* 返回：原始采样 × 当前变比 */
////extern float PT100_GetTemperature(void);

///* DAC */
////extern void DAC_SetOutput(uint16_t value); /* 0~4095 */

///* 系统 */
//extern void mcu_software_reset(void);
//extern void delay_1ms(uint32_t ms);
////extern void EnterDeepSleep(void);       /* 深度睡眠+RTC闹钟10s唤醒 */

///* 串口波特率切换 */
////extern void UART_SetBaudrateByCode(uint8_t code); /* 11/12/13/14 */

///* Flash 参数持久化（在 Bootloader/App 的参数区操作） */
////extern void Param_LoadAll(void);        /* 从Flash加载到本文件静态变量 */
////extern void Param_SaveAll(void);        /* 保存本文件静态变量到Flash */

///* 升级触发（设置Flash升级标志+软复位） */
////extern void Bootloader_TriggerUpgrade(void);

///* 获取RTC日期时间字符串 "2026-01-01 12:00:00" */
////extern void RTC_GetDateTimeString(char *out, uint16_t len);

///* ===================== 静态运行参数 ===================== */
//static uint16_t s_device_id       = 0x0001;
//static uint8_t  s_baudrate_code   = 0x13;         /* 默认19200 */
//static float    s_ch0_ratio       = 1.0f;
//static float    s_ch1_ratio       = 1.0f;
//static float    s_ch0_threshold   = 0.0f;
//static float    s_ch1_threshold   = 0.0f;
//static uint16_t s_report_interval_ms = 1000;      /* 默认1s */
//static uint8_t  s_alarm_mode      = 0x02;         /* 02=不主动上报（默认安全） */

///* 自动上报状态 */
//static uint8_t  s_is_auto_reporting = 0;

///* 告警记录环形缓冲区（最近10条） */
//typedef struct {
//    char    datetime[20];   /* "2026-01-01 12:00:00" */
//    uint8_t channel;        /* 0 or 1 */
//    float   threshold;
//    float   actual;
//} AlarmRecord_t;

//static AlarmRecord_t s_alarm_records[10];
//static uint8_t       s_alarm_wr_idx = 0;  /* 下一个写入位置 */
//static uint8_t       s_alarm_count  = 0;  /* 当前有效条数 */

///* ===================== 工具函数 ===================== */
//static void SendOK(uint16_t dev_id, uint16_t cmd)
//{
//    uint8_t ok = 0xFF;
//    Protocol_SendAck(dev_id, cmd, &ok, 1);
//}

//static void FloatToBytes(float val, uint8_t *out)
//{
//    union { float f; uint8_t b[4]; } u;
//    u.f = val;
//    out[0] = u.b[3];  /* 大端 */
//    out[1] = u.b[2];
//    out[2] = u.b[1];
//    out[3] = u.b[0];
//}

//static float BytesToFloat(const uint8_t *in)
//{
//    union { float f; uint8_t b[4]; } u;
//    u.b[0] = in[3];
//    u.b[1] = in[2];
//    u.b[2] = in[1];
//    u.b[3] = in[0];
//    return u.f;
//}

///* ===================== 接口实现 ===================== */
//void CmdHandler_Init(void)
//{
////    Param_LoadAll();    /* 从Flash加载，覆盖上述默认值 */
//}

//uint16_t CmdHandler_GetDeviceID(void)
//{
//    return s_device_id;
//}

//uint8_t CmdHandler_IsAutoReporting(void)
//{
//    return s_is_auto_reporting;
//}

//uint16_t CmdHandler_GetReportIntervalMs(void)
//{
//    return s_report_interval_ms;
//}

///* 构建自动上报的12字节内容：UTC(4)+CH0(4)+CH1(4) */
//uint8_t CmdHandler_BuildAutoReportData(uint8_t *out_12bytes)
//{
////    uint32_t utc = RTC_GetUTC();
////    float ch0 = ADC_GetCH0_Value();
////    float ch1 = ADC_GetCH1_Value();

////    out_12bytes[0] = (utc >> 24) & 0xFF;
////    out_12bytes[1] = (utc >> 16) & 0xFF;
////    out_12bytes[2] = (utc >>  8) & 0xFF;
////    out_12bytes[3] = (utc      ) & 0xFF;

////    FloatToBytes(ch0, &out_12bytes[4]);
////    FloatToBytes(ch1, &out_12bytes[8]);
//    return 12;
//}

///* 添加一条告警记录 */
//void CmdHandler_AddAlarmRecord(const char *datetime_str, uint8_t ch,
//                                float threshold, float actual)
//{
//    AlarmRecord_t *rec = &s_alarm_records[s_alarm_wr_idx];
//    strncpy(rec->datetime, datetime_str, sizeof(rec->datetime)-1);
//    rec->datetime[sizeof(rec->datetime)-1] = '\0';
//    rec->channel   = ch;
//    rec->threshold = threshold;
//    rec->actual    = actual;

//    s_alarm_wr_idx = (s_alarm_wr_idx + 1) % 10;
//    if (s_alarm_count < 10) s_alarm_count++;
//}

///* ===================== 命令分发 ===================== */
//void CmdHandler_ProcessFrame(const ProtocolFrame_t *frame)
//{
//    if (frame == NULL) return;

//    /* 1. 设备ID过滤：非广播且不匹配，静默丢弃 */
//    if (frame->device_id != 0xFFFF && frame->device_id != s_device_id) {
//        return;
//    }

//    /* 2. 自动上报隔离：除停止命令外，其他命令一律不应答 */
//    if (s_is_auto_reporting && frame->cmd_word != 0x0303) {
//        return;
//    }

//    /* 3. 命令分发 */
//    switch (frame->cmd_word) {
//        /* ---------- 系统管理类 ---------- */
//        case 0x0101:  /* 设备重启 */
//            SendOK(s_device_id, 0x0101);
//            delay_1ms(100);
//            mcu_software_reset();
//            break;

//        case 0x0104: { /* 查询固件版本 2.0.1.0 -> 00020001 */
////            uint8_t ver[4] = {0x00, 0x02, 0x00, 0x01};
////            Protocol_SendAck(s_device_id, 0x0104, ver, 4);
//            break;
//        }

//        case 0x0105: { /* 设置时间 */
////            if (frame->data_len != 4) goto L_ERR;
////            uint32_t utc = ((uint32_t)frame->data[0] << 24) |
////                           ((uint32_t)frame->data[1] << 16) |
////                           ((uint32_t)frame->data[2] <<  8) |
////                            frame->data[3];
////            RTC_SetUTC(utc);
////            Param_SaveAll();
////            SendOK(s_device_id, 0x0105);
//            break;
//        }

//        case 0x0106: { /* 查询时间 */
////            uint32_t utc = RTC_GetUTC();
////            uint8_t d[4] = {(utc>>24)&0xFF, (utc>>16)&0xFF, (utc>>8)&0xFF, utc&0xFF};
////            Protocol_SendAck(s_device_id, 0x0106, d, 4);
//            break;
//        }

//        case 0x01A1: { /* 设置设备ID */
////            if (frame->data_len != 2) goto L_ERR;
////            uint16_t new_id = ((uint16_t)frame->data[0] << 8) | frame->data[1];
////            if (new_id == 0x0000 || new_id == 0xFFFF) goto L_ERR;
////            s_device_id = new_id;
////            Param_SaveAll();
////            /* 注意：应答使用新ID */
////            SendOK(s_device_id, 0x01A1);
//            break;
//        }

//        case 0x0111: { /* 查询设备ID */
////            uint8_t d[2] = {(s_device_id >> 8) & 0xFF, s_device_id & 0xFF};
////            Protocol_SendAck(s_device_id, 0x0111, d, 2);
//            break;
//        }

//        case 0x01A2: { /* 设置波特率 */
////            if (frame->data_len != 1) goto L_ERR;
////            uint8_t code = frame->data[0];
////            if (code != 0x11 && code != 0x12 && code != 0x13 && code != 0x14) goto L_ERR;
////            s_baudrate_code = code;
////            Param_SaveAll();
////            SendOK(s_device_id, 0x01A2);
////            delay_1ms(50);
////            UART_SetBaudrateByCode(code);   /* 立即生效 */
//            break;
//        }

//        case 0x0112: { /* 查询波特率 */
//            Protocol_SendAck(s_device_id, 0x0112, &s_baudrate_code, 1);
//            break;
//        }

//        /* ---------- 数据类 ---------- */
//        case 0x0201: { /* 查询CH0 */
////            float val = ADC_GetCH0_Value();
////            uint8_t d[4]; FloatToBytes(val, d);
////            Protocol_SendAck(s_device_id, 0x0201, d, 4);
//            break;
//        }

//        case 0x0202: { /* 查询CH1 */
////            float val = ADC_GetCH1_Value();
////            uint8_t d[4]; FloatToBytes(val, d);
////            Protocol_SendAck(s_device_id, 0x0202, d, 4);
//            break;
//        }

//        case 0x0221: { /* 查询CH2 PT100 */
////            float val = PT100_GetTemperature();
////            uint8_t d[4]; FloatToBytes(val, d);
////            Protocol_SendAck(s_device_id, 0x0221, d, 4);
//            break;
//        }

//        case 0x0241: { /* 设置CH0变比 */
////            if (frame->data_len != 4) goto L_ERR;
////            s_ch0_ratio = BytesToFloat(frame->data);
////            Param_SaveAll();
////            SendOK(s_device_id, 0x0241);
//            break;
//        }

//        case 0x0242: { /* 设置CH1变比 */
////            if (frame->data_len != 4) goto L_ERR;
////            s_ch1_ratio = BytesToFloat(frame->data);
////            Param_SaveAll();
////            SendOK(s_device_id, 0x0242);
//            break;
//        }

//        case 0x0261: { /* 设置上报间隔 */
////            if (frame->data_len != 1) goto L_ERR;
////            uint8_t code = frame->data[0];
////            if (code == 0x01)      s_report_interval_ms = 1000;
////            else if (code == 0x02) s_report_interval_ms = 3000;
////            else if (code == 0x03) s_report_interval_ms = 5000;
////            else goto L_ERR;
////            Param_SaveAll();
////            SendOK(s_device_id, 0x0261);
//            break;
//        }

//        /* ---------- 控制类 ---------- */
//        case 0x0301: { /* 设置DAC */
////            if (frame->data_len != 2) goto L_ERR;
////            uint16_t dac = ((uint16_t)frame->data[0] << 8) | frame->data[1];
////            if (dac > 4095) goto L_ERR;
////            DAC_SetOutput(dac);
////            SendOK(s_device_id, 0x0301);
//            break;
//        }

//        case 0x0302: { /* 开始自动上报 */
////            SendOK(s_device_id, 0x0302);
////            s_is_auto_reporting = 1;
//            break;
//        }

//        case 0x0303: { /* 停止自动上报 */
////            s_is_auto_reporting = 0;
////            SendOK(s_device_id, 0x0303);
//            break;
//        }

//        case 0x03AA: { /* 睡眠 */
////            SendOK(s_device_id, 0x03AA);
////            delay_1ms(50);
////            EnterDeepSleep();   /* 10秒后RTC唤醒，唤醒代码需发送"instrument wakeup" */
//            break;
//        }

//        /* ---------- 参数配置类 ---------- */
//        case 0x0400: { /* 批量读取阈值 CH0+CH1 */
////            uint8_t d[8];
////            FloatToBytes(s_ch0_threshold, &d[0]);
////            FloatToBytes(s_ch1_threshold, &d[4]);
////            Protocol_SendAck(s_device_id, 0x0400, d, 8);
//            break;
//        }

//        case 0x0401: { /* 读取CH0阈值 */
////            uint8_t d[4]; FloatToBytes(s_ch0_threshold, d);
////            Protocol_SendAck(s_device_id, 0x0401, d, 4);
//            break;
//        }

//        case 0x0402: { /* 读取CH1阈值 */
////            uint8_t d[4]; FloatToBytes(s_ch1_threshold, d);
////            Protocol_SendAck(s_device_id, 0x0402, d, 4);
//            break;
//        }

//        case 0x0411: { /* 写入CH0阈值 */
////            if (frame->data_len != 4) goto L_ERR;
////            s_ch0_threshold = BytesToFloat(frame->data);
////            Param_SaveAll();
////            SendOK(s_device_id, 0x0411);
//            break;
//        }

//        case 0x0412: { /* 写入CH1阈值 */
////            if (frame->data_len != 4) goto L_ERR;
////            s_ch1_threshold = BytesToFloat(frame->data);
////            Param_SaveAll();
////            SendOK(s_device_id, 0x0412);
//            break;
//        }

//        /* ---------- 系统升级类 ---------- */
//        case 0x0501: { /* 升级请求 */
////            SendOK(s_device_id, 0x0501);
////            delay_1ms(100);
////            Bootloader_TriggerUpgrade(); /* 写Flash标志+软复位 */
//            break;
//        }

//        case 0x0502:  /* App中收到：拒绝 */
//        case 0x0503:
////            Protocol_SendError(s_device_id);
////            break;

//        /* ---------- 告警与日志类 ---------- */
//        case 0x0601: { /* 设置告警模式 */
////            if (frame->data_len != 1) goto L_ERR;
////            uint8_t mode = frame->data[0];
////            if (mode != 0x01 && mode != 0x02) goto L_ERR;
////            s_alarm_mode = mode;
//////            Param_SaveAll();
////            SendOK(s_device_id, 0x0601);
//            break;
//        }

//        case 0x0602: { /* 查询告警记录 —— 直接发送字符串，不经帧封装！ */
////            if (s_alarm_count == 0) {
////                RS485_SendData((uint8_t*)"empty", 5);
////            } else {
////                char line[64];
////                uint8_t idx = (s_alarm_wr_idx + 10 - s_alarm_count) % 10; /* 最旧的一条 */
////                /* 按时间倒序输出：从新到旧 */
////                for (int8_t i = s_alarm_count - 1; i >= 0; i--) {
////                    uint8_t r = (s_alarm_wr_idx + 10 - 1 - i) % 10;
////                    AlarmRecord_t *rec = &s_alarm_records[r];
////                    snprintf(line, sizeof(line), "%s|CH%d|%.2f|%.2f\r\n",
////                             rec->datetime, rec->channel, rec->threshold, rec->actual);
////                    RS485_SendData((uint8_t*)line, strlen(line));
////                }
////            }
//            break;
//        }

//        case 0x0603: { /* 清除告警 */
////            s_alarm_count = 0;
////            s_alarm_wr_idx = 0;
////            memset(s_alarm_records, 0, sizeof(s_alarm_records));
////            Param_SaveAll();
////            SendOK(s_device_id, 0x0603);
//            break;
//        }

//        /* ---------- 特殊命令 ---------- */
//        case 0xFFFF: { /* 广播寻找设备 */
////            Protocol_SendHeartbeat(s_device_id);
//            break;
//        }

//        default:
////		L_ERR:
////            Protocol_SendError(s_device_id);
//            break;
//    }
//}