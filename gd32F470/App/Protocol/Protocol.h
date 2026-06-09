#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*================ 帧类型定义 =================*/
#define FRAME_TYPE_CMD        0x01   /* 命令帧 */
#define FRAME_TYPE_ACK        0x02   /* 应答帧 */
#define FRAME_TYPE_ERROR      0x03   /* 错误帧（预留） */
#define FRAME_TYPE_HEART      0x05   /* 心跳/广播帧 */

/*================ 协议常量 =================*/
#define PROTOCOL_VERSION      0x02   /* 协议版本（从示例确认） */
#define CMD_ERROR_GENERAL     0xEEEE  /* 通用错误命令字 */

/*================ 特殊命令字 =================*/
#define CMD_HEARTBEAT         0x8888  /* 设备心跳/上电通知（帧类型0x05） */
#define CMD_BROADCAST_FIND    0xFFFF  /* 上位机广播寻找设备（帧类型0x05） */

/*================ 错误码定义 =================*/
#define ERR_CODE_CRC_FAIL     0x0001  /* CRC校验失败 */
#define ERR_CODE_LENGTH_ERR   0x0002  /* 帧长度不匹配 */
#define ERR_CODE_UNKNOWN_CMD  0x0003  /* 未知帧类型/命令字 */

/*================ 帧结构体 =================*/
typedef struct {
    uint16_t device_id;
    uint8_t  frame_type;
    uint16_t cmd_word;
    uint8_t  data_len;
    uint8_t  data[128];
    uint16_t crc16;
} ProtocolFrame_t;

/*================ 函数声明 =================*/
uint16_t CRC16_Modbus(const uint8_t *data, uint16_t len);
bool Protocol_ParseFrame(ProtocolFrame_t *frame);
void Protocol_SendAck(uint16_t device_id, uint16_t cmd_word,
                      const uint8_t *data, uint8_t data_len);
void Protocol_SendFrame(uint16_t device_id, uint8_t frame_type, uint16_t cmd_word,
                               const uint8_t *data, uint8_t data_len);

void Protocol_SendError(uint16_t device_id);
void Protocol_SendHeartbeat(uint16_t device_id);

/* 发送错误应答帧（帧类型 0xFF，命令字 0xEEEE，无payload）— 定义在 Command.c */
void Protocol_SendErrorFrame(uint16_t device_id);

#endif
