#ifndef __COMMAND_H
#define __COMMAND_H
#include "Protocol.h"
/*================ 系统管理类 (0x01) =================*/
#define CMD_DEVICE_REBOOT       0x0101  /* 设备重启 */
#define CMD_FACTORY_RESET       0x0102  /* 恢复出厂设置（预留，初赛不考察） */
#define CMD_QUERY_DEVICE_INFO   0x0103  /* 查询设备信息（预留，初赛不考察） */
#define CMD_QUERY_FW_VERSION    0x0104  /* 查询固件版本 */
#define CMD_SET_TIME            0x0105  /* 设置设备时间 */
#define CMD_QUERY_TIME          0x0106  /* 查询设备时间 */
#define CMD_SET_DEVICE_ID       0x01A1  /* 设置设备ID */
#define CMD_SET_BAUDRATE        0x01A2  /* 设置波特率 */
#define CMD_QUERY_DEVICE_ID     0x0111  /* 查询设备ID */
#define CMD_QUERY_BAUDRATE      0x0112  /* 查询波特率 */
/*================ 数据类 (0x02) =================*/
#define CMD_QUERY_CH0           0x0201  /* 查询CH0数据（ADC通道0：滑动变阻器） */
#define CMD_QUERY_CH1           0x0202  /* 查询CH1数据（ADC通道1：DAC回读） */
#define CMD_QUERY_CH2_PT100     0x0221  /* 查询特定通道数据（外部ADC的PT100） */
#define CMD_SET_CH0_RATIO       0x0241  /* 设置CH0变比 */
#define CMD_SET_CH1_RATIO       0x0242  /* 设置CH1变比 */
#define CMD_SET_REPORT_INTERVAL 0x0261  /* 设置数据上报时间间隔 */
/*================ 控制类 (0x03) =================*/
#define CMD_SET_DAC_OUTPUT      0x0301  /* 设置DAC输出电压 */
#define CMD_START_AUTO_REPORT   0x0302  /* 定时自动上报数据开始（批量上报CH0、CH1） */
#define CMD_STOP_AUTO_REPORT    0x0303  /* 定时自动上报数据停止 */
#define CMD_ENTER_SLEEP         0x03AA  /* 进入睡眠模式（MCU深度睡眠，RTC闹钟10s后唤醒） */
/*================ 参数配置类 (0x04) =================*/
#define CMD_READ_THRESHOLDS     0x0400  /* 读取阈值参数（批量读取CH0、CH1） */
#define CMD_READ_CH0_THRESHOLD  0x0401  /* 读取CH0阈值参数 */
#define CMD_READ_CH1_THRESHOLD  0x0402  /* 读取CH1阈值参数 */
#define CMD_READ_CH2_THRESHOLD  0x0403  /* 读取CH2阈值参数 */
#define CMD_WRITE_CH0_THRESHOLD 0x0411  /* 写入CH0阈值参数 */
#define CMD_WRITE_CH1_THRESHOLD 0x0412  /* 写入CH1阈值参数 */
#define CMD_WRITE_CH2_THRESHOLD 0x0413  /* 写入CH2阈值参数 */
/*================ 系统升级类 (0x05) =================*/
#define CMD_UPGRADE_REQUEST     0x0501  /* 升级请求：APP收到后写标志并复位进入Bootloader */
#define CMD_PREPARE_FIRMWARE    0x0502  /* 准备传输固件数据包：Bootloader收到后准备接收bin */
#define CMD_EXECUTE_UPGRADE     0x0503  /* 执行升级流程：Bootloader收到后搬运固件到APP区 */
/*================ 告警与日志类 (0x06) =================*/
#define CMD_SET_ALARM_MODE      0x0601  /* 设置是否主动上报告警（01主动上报，02仅存储） */
#define CMD_QUERY_ALARM_RECORD  0x0602  /* 查询告警记录（字符串直接回复，非帧封装） */
#define CMD_CLEAR_ALARM_RECORD  0x0603  /* 清除告警记录 */


/* 发送错误应答帧（帧类型 0xFF，命令字 0xEEEE，data_len=0，无payload） */
void Protocol_SendErrorFrame(uint16_t device_id);
void Cmd_AutoReportTick(uint16_t device_id);
/*================ 命令分发入口 =================*/
void Cmd_ProcessFrame(ProtocolFrame_t *frame);
/*================ 各命令处理函数声明 =================*/
/*--- 系统管理类 ---*/
void Cmd_DeviceReboot(ProtocolFrame_t *frame);        /* 0x0101 */
void Cmd_FactoryReset(ProtocolFrame_t *frame);        /* 0x0102 预留 */
void Cmd_QueryDeviceInfo(ProtocolFrame_t *frame);     /* 0x0103 预留 */
void Cmd_QueryFwVersion(ProtocolFrame_t *frame);      /* 0x0104 */
void Cmd_SetTime(ProtocolFrame_t *frame);             /* 0x0105 */
void Cmd_QueryTime(ProtocolFrame_t *frame);           /* 0x0106 */
void Cmd_SetDeviceID(ProtocolFrame_t *frame);         /* 0x01A1 */
void Cmd_SetBaudrate(ProtocolFrame_t *frame);         /* 0x01A2 */
void Cmd_QueryDeviceID(ProtocolFrame_t *frame);       /* 0x0111 */
void Cmd_QueryBaudrate(ProtocolFrame_t *frame);       /* 0x0112 */
/*--- 数据类 ---*/
void Cmd_QueryCh0(ProtocolFrame_t *frame);            /* 0x0201 */
void Cmd_QueryCh1(ProtocolFrame_t *frame);            /* 0x0202 */
void Cmd_QueryCh2Pt100(ProtocolFrame_t *frame);        /* 0x0221 */
void Cmd_SetCh0Ratio(ProtocolFrame_t *frame);          /* 0x0241 */
void Cmd_SetCh1Ratio(ProtocolFrame_t *frame);          /* 0x0242 */
void Cmd_SetReportInterval(ProtocolFrame_t *frame);    /* 0x0261 */
/*--- 控制类 ---*/
void Cmd_SetDacOutput(ProtocolFrame_t *frame);         /* 0x0301 */
void Cmd_StartAutoReport(ProtocolFrame_t *frame);      /* 0x0302 */
void Cmd_StopAutoReport(ProtocolFrame_t *frame);       /* 0x0303 */
void Cmd_EnterSleep(ProtocolFrame_t *frame);           /* 0x03AA */
/*--- 参数配置类 ---*/
void Cmd_ReadThresholds(ProtocolFrame_t *frame);       /* 0x0400 */
void Cmd_ReadCh0Threshold(ProtocolFrame_t *frame);    /* 0x0401 */
void Cmd_ReadCh1Threshold(ProtocolFrame_t *frame);    /* 0x0402 */
void Cmd_ReadCh2Threshold(ProtocolFrame_t *frame);    /* 0x0403 */
void Cmd_WriteCh0Threshold(ProtocolFrame_t *frame);   /* 0x0411 */
void Cmd_WriteCh1Threshold(ProtocolFrame_t *frame);   /* 0x0412 */
void Cmd_WriteCh2Threshold(ProtocolFrame_t *frame);   /* 0x0413 */
/*--- 系统升级类 ---*/
void Cmd_UpgradeRequest(ProtocolFrame_t *frame);      /* 0x0501 */
void Cmd_PrepareFirmware(ProtocolFrame_t *frame);     /* 0x0502 */
void Cmd_ExecuteUpgrade(ProtocolFrame_t *frame);       /* 0x0503 */
/*--- 告警与日志类 ---*/
void Cmd_SetAlarmMode(ProtocolFrame_t *frame);         /* 0x0601 */
void Cmd_QueryAlarmRecord(ProtocolFrame_t *frame);     /* 0x0602 */
void Cmd_ClearAlarmRecord(ProtocolFrame_t *frame);     /* 0x0603 */
void Alarm_CheckAndReport(uint16_t device_id, uint8_t channel, float actual, float threshold);
/*--- 特殊帧 ---*/
void Cmd_Heartbeat(ProtocolFrame_t *frame);           /* 0x8888 心跳 */
void Cmd_BroadcastFind(ProtocolFrame_t *frame);       /* 0xFFFF 广播寻址 */
#endif

