/************************************************************
 * ©2026CIMC Copyright©
 * 文件名: Protocol.h
 * 说明: 通讯协议层 - 帧格式、命令定义、接口
 * 作者: Copilot
 * 版本: V1.0 2026/06/06
************************************************************/

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "gd32f4xx.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/*======================== 协议常量定义 ========================*/
#define FRAME_HEADER          0xA5B6
#define FRAME_TAIL            0xB6A5
#define FRAME_VERSION         0x02
#define BROADCAST_ADDR        0xFFFF
#define PROTOCOL_TIMEOUT_MS   2000

/* 帧类型 */
#define FRAME_TYPE_CMD        0x01   /* 上位机→设备 命令下发帧 */
#define FRAME_TYPE_ACK        0x02   /* 设备→上位机 应答帧 */
#define FRAME_TYPE_HEART      0x05   /* 双向 心跳帧 */
#define FRAME_TYPE_ERROR      0xFF   /* 设备→上位机 异常报警帧 */

/* 特殊命令字 */
#define CMD_HEARTBEAT         0x8888 /* 设备上电/复位后发送 */
#define CMD_BROADCAST_FIND    0xFFFF /* 上位机广播寻找设备 */
#define CMD_ERROR_RESP        0xEEEE /* 错误应答 */

/* 命令字定义 */
#define CMD_REBOOT            0x0101
#define CMD_FACTORY_RESET     0x0102
#define CMD_DEV_INFO          0x0103
#define CMD_FW_VERSION        0x0104
#define CMD_SET_TIME          0x0105
#define CMD_GET_TIME          0x0106
#define CMD_SET_DEV_ID        0x01A1
#define CMD_SET_BAUDRATE      0x01A2
#define CMD_GET_DEV_ID        0x0111
#define CMD_GET_BAUDRATE      0x0112

#define CMD_CH0_DATA          0x0201
#define CMD_CH1_DATA          0x0202
#define CMD_CH2_DATA          0x0221
#define CMD_SET_CH0_RATIO     0x0241
#define CMD_SET_CH1_RATIO     0x0242
#define CMD_SET_REPORT_INTERVAL 0x0261

#define CMD_SET_DAC           0x0301
#define CMD_START_AUTO_REPORT 0x0302
#define CMD_STOP_AUTO_REPORT  0x0303
#define CMD_SLEEP_MODE        0x03AA

#define CMD_GET_THRESHOLD     0x0400
#define CMD_GET_CH0_THRESHOLD 0x0401
#define CMD_GET_CH1_THRESHOLD 0x0402
#define CMD_GET_CH2_THRESHOLD 0x0403
#define CMD_SET_CH0_THRESHOLD 0x0411
#define CMD_SET_CH1_THRESHOLD 0x0412
#define CMD_SET_CH2_THRESHOLD 0x0413

#define CMD_UPGRADE_REQUEST   0x0501
#define CMD_PREPARE_FW        0x0502
#define CMD_EXECUTE_UPGRADE   0x0503

#define CMD_ENABLE_ALARM      0x0601
#define CMD_QUERY_ALARM       0x0602
#define CMD_CLEAR_ALARM       0x0603
#define CMD_QUERY_LOG         0x0604
#define CMD_CLEAR_LOG         0x0605

/*======================== 数据结构 ========================*/

/* 接收到的协议帧 */
typedef struct {
    uint16_t frame_header;      /* 起始标志 0xA5B6 */
    uint16_t device_id;         /* 设备ID */
    uint8_t  frame_type;        /* 帧类型 */
    uint16_t cmd_word;          /* 命令字 */
    uint8_t  data_len;          /* 内容长度 */
    uint8_t  protocol_version;  /* 协议版本 */
    uint8_t  data[256];         /* 内容数据 */
    uint16_t crc16;             /* CRC校验值 */
    uint16_t frame_tail;        /* 结束标志 0xB6A5 */
    uint8_t  valid;             /* 是否有效 */
} ProtocolFrame_t;

/*======================== 函数接口 ========================*/

/* 初始化协议层 */
void Protocol_Init(void);

/* 解析接收缓冲区中的帧数据
   返回: 1-成功解析一帧，0-缓冲区数据不足或出错 */
uint8_t Protocol_ParseFrame(ProtocolFrame_t *frame);

/* 发送应答帧 (应答内容为0xFF时表示OK) */
void Protocol_SendAck(uint16_t device_id, uint16_t cmd_word, 
                      const uint8_t *data, uint8_t len);

/* 发送错误应答帧 */
void Protocol_SendError(uint16_t device_id, uint16_t cmd_word);

/* 发送心跳帧 */
void Protocol_SendHeartbeat(uint16_t device_id);

/* 发送原始数据帧 (低级接口) */
void Protocol_SendRawFrame(uint16_t device_id, uint8_t frame_type,
                           uint16_t cmd_word, const uint8_t *data, uint8_t len);

/* CRC-16-Modbus 校验 */
uint16_t Protocol_CalcCRC16(const uint8_t *data, uint16_t len);

/* 获取接收缓冲区状态 */
uint16_t Protocol_GetRxCount(void);
void Protocol_RxReset(void);

#endif

/****************************End*****************************/
