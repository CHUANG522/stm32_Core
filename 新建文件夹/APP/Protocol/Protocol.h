#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "HeaderFiles.h"

#define PROTOCOL_VERSION    0x02

/* 帧类型 */
#define FRAME_TYPE_CMD      0x01    // 上位机命令
#define FRAME_TYPE_ACK      0x02    // 设备应答
#define FRAME_TYPE_HEART    0x05    // 心跳
#define FRAME_TYPE_ERROR    0xFF    // 错误/异常

/* 特殊命令字 */
#define CMD_BROADCAST_FIND  0xFFFF
#define CMD_HEARTBEAT       0x8888
#define CMD_ERROR_GENERAL   0xEEEE

/* 解析后的帧结构 */
typedef struct {
    uint16_t device_id;     // 设备ID
    uint8_t  frame_type;    // 帧类型
    uint16_t cmd_word;      // 命令字
    uint8_t  data_len;      // 内容长度（字节数）
    uint8_t  data[128];     // 内容数据
    uint16_t crc16;         // 接收到的CRC
} ProtocolFrame_t;

/* CRC16-Modbus */
uint16_t CRC16_Modbus(const uint8_t *data, uint16_t len);

/* 从环形缓冲区解析一帧 */
bool Protocol_ParseFrame(ProtocolFrame_t *frame);

/* 组帧并发送（帧类型0x02 应答） */
void Protocol_SendAck(uint16_t device_id, uint16_t cmd_word, 
                      const uint8_t *data, uint8_t data_len);

/* 组帧并发送错误帧（帧类型0xFF） */
void Protocol_SendError(uint16_t device_id);

/* 组帧并发送心跳帧 */
void Protocol_SendHeartbeat(uint16_t device_id);

#endif

