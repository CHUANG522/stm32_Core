#ifndef __CMD_HANDLER_H
#define __CMD_HANDLER_H


#include "HeaderFiles.h"





/* 初始化（加载 Flash 参数） */
void CmdHandler_Init(void);

/* 主循环调用：处理一帧数据 */
void CmdHandler_ProcessFrame(const ProtocolFrame_t *frame);////

/* 自动上报相关（主循环调用） */
uint8_t  CmdHandler_IsAutoReporting(void);
uint16_t CmdHandler_GetReportIntervalMs(void);
uint8_t  CmdHandler_BuildAutoReportData(uint8_t *out_12bytes); /* 返回12 */

/* 告警记录添加（业务层检测到超阈值时调用） */
void CmdHandler_AddAlarmRecord(const char *datetime_str, uint8_t ch, 
                                float threshold, float actual);

/* 当前设备ID（主循环/Protocol发送心跳用） */
uint16_t CmdHandler_GetDeviceID(void);

#endif


