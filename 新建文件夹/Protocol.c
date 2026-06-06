/************************************************************
 * ©2026CIMC Copyright©
 * 文件名: Protocol.c
 * 说明: 通讯协议层实现 - 帧解析、校验、发送
 * 作者: Copilot
 * 版本: V1.1 2026/06/06 (修复编译警告)
************************************************************/

#include "Protocol.h"
#include "USART1.h"

/*======================== 状态机定义 ========================*/
typedef enum {
    PARSE_STATE_IDLE = 0,           /* 等待帧头第一字节 */
    PARSE_STATE_HEADER_BYTE2,       /* 等待帧头第二字节 */
    PARSE_STATE_DEVICE_ID_BYTE1,    /* 等待设备ID高字节 */
    PARSE_STATE_DEVICE_ID_BYTE2,    /* 等待设备ID低字节 */
    PARSE_STATE_FRAME_TYPE,         /* 等待帧类型 */
    PARSE_STATE_CMD_BYTE1,          /* 等待命令字高字节 */
    PARSE_STATE_CMD_BYTE2,          /* 等待命令字低字节 */
    PARSE_STATE_DATA_LEN,           /* 等待数据长度 */
    PARSE_STATE_PROTOCOL_VER,       /* 等待协议版本 */
    PARSE_STATE_DATA,               /* 等待内容数据 */
    PARSE_STATE_CRC_BYTE1,          /* 等待CRC高字节 */
    PARSE_STATE_CRC_BYTE2,          /* 等待CRC低字节 */
    PARSE_STATE_TAIL_BYTE1,         /* 等待帧尾高字节 */
    PARSE_STATE_TAIL_BYTE2          /* 等待帧尾低字节 */
} ParseState_t;

/*======================== 全局变量 ========================*/
static ParseState_t g_parse_state = PARSE_STATE_IDLE;
static ProtocolFrame_t g_frame_buffer = {0};
static uint8_t g_data_index = 0;

/*======================== CRC-16-Modbus实现 ========================*/
uint16_t Protocol_CalcCRC16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i, j;
    
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

/*======================== 帧组装和发送 ========================*/

/* 构建完整帧的二进制数据 */
static uint16_t Protocol_BuildFrame(uint8_t *buf, uint16_t device_id, 
                                    uint8_t frame_type, uint16_t cmd_word,
                                    const uint8_t *data, uint8_t data_len)
{
    uint16_t pos = 0;
    uint16_t crc;
    
    /* 帧头: 0xA5B6 */
    buf[pos++] = 0xA5;
    buf[pos++] = 0xB6;
    
    /* 设备ID: 大端序 */
    buf[pos++] = (device_id >> 8) & 0xFF;
    buf[pos++] = device_id & 0xFF;
    
    /* 帧类型 */
    buf[pos++] = frame_type;
    
    /* 命令字: 大端序 */
    buf[pos++] = (cmd_word >> 8) & 0xFF;
    buf[pos++] = cmd_word & 0xFF;
    
    /* 报文长度 (只包含内容N字节) */
    buf[pos++] = data_len;
    
    /* 协议版本 */
    buf[pos++] = FRAME_VERSION;
    
    /* 内容数据 */
    if (data_len > 0 && data != NULL) {
        memcpy(&buf[pos], data, data_len);
        pos += data_len;
    }
    
    /* CRC-16-Modbus (对[起始标志~内容]计算，大端序) */
    crc = Protocol_CalcCRC16(buf, pos);
    buf[pos++] = (crc >> 8) & 0xFF;
    buf[pos++] = crc & 0xFF;
    
    /* 帧尾: 0xB6A5 */
    buf[pos++] = 0xB6;
    buf[pos++] = 0xA5;
    
    return pos;
}

/* 转换二进制帧为ASCII字符串并发送 */
static void Protocol_SendFrameASCII(uint8_t *frame_bin, uint16_t frame_len)
{
    char ascii_buf[512];
    uint16_t i;
    
    /* 将二进制转为ASCII十六进制字符串 */
    for (i = 0; i < frame_len; i++) {
        sprintf(&ascii_buf[i*2], "%02X", frame_bin[i]);
    }
    
    /* 发送ASCII字符串 */
    RS485_SendData((uint8_t*)ascii_buf, frame_len * 2);
}

void Protocol_SendRawFrame(uint16_t device_id, uint8_t frame_type,
                           uint16_t cmd_word, const uint8_t *data, uint8_t len)
{
    uint8_t frame_bin[512];
    uint16_t frame_len;
    
    frame_len = Protocol_BuildFrame(frame_bin, device_id, frame_type, 
                                    cmd_word, data, len);
    Protocol_SendFrameASCII(frame_bin, frame_len);
}

void Protocol_SendAck(uint16_t device_id, uint16_t cmd_word,
                      const uint8_t *data, uint8_t len)
{
    Protocol_SendRawFrame(device_id, FRAME_TYPE_ACK, cmd_word, data, len);
}

void Protocol_SendError(uint16_t device_id, uint16_t cmd_word)
{
    uint8_t frame_bin[512];
    uint16_t frame_len;
    
    /* 构建错误应答帧 (帧类型0xFF, 命令字原始) */
    frame_len = Protocol_BuildFrame(frame_bin, device_id, FRAME_TYPE_ERROR, 
                                    cmd_word, NULL, 0);
    Protocol_SendFrameASCII(frame_bin, frame_len);
}

void Protocol_SendHeartbeat(uint16_t device_id)
{
    Protocol_SendRawFrame(device_id, FRAME_TYPE_HEART, CMD_HEARTBEAT, NULL, 0);
}

/*======================== ASCII字符串转十六进制 ========================*/
static uint8_t Protocol_AsciiToHex(char c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0xFF;  /* 非法字符 */
}

static uint8_t Protocol_Ascii2BytesToByte(const char *ascii)
{
    uint8_t high = Protocol_AsciiToHex(ascii[0]);
    uint8_t low = Protocol_AsciiToHex(ascii[1]);
    
    if (high == 0xFF || low == 0xFF) return 0xFF;
    return (uint8_t)((high << 4) | low);
}

/*======================== 帧解析状态机 ========================*/

uint8_t Protocol_ParseFrame(ProtocolFrame_t *frame)
{
    static uint16_t ascii_count = 0;
    static char ascii_buf[512];
    uint8_t byte_val;
    uint16_t i;
    
    /* 首次调用时，从接收缓冲区读取ASCII字符串 */
    if (g_parse_state == PARSE_STATE_IDLE) {
        uint16_t count = Usart1Ring_GetCount();
        
        if (count < 18) {  /* 最小帧长度 (不含内容数据) */
            return 0;
        }
        
        /* 查找帧头"A5B6" */
        uint8_t peek_buf[512];
        uint16_t frame_end = 0;
        
        Usart1Ring_Peek(peek_buf, count);
        
        /* 查找帧头ASCII字符串 "A5B6" */
        for (i = 0; i + 3 < count; i++) {
            if (peek_buf[i] == 'A' && peek_buf[i+1] == '5' &&
                peek_buf[i+2] == 'B' && peek_buf[i+3] == '6') {
                break;  /* 找到帧头 */
            }
        }
        
        if (i + 3 >= count) {
            /* 未找到帧头，跳过第一个字符 */
            Usart1Ring_Skip(1);
            return 0;
        }
        
        /* 跳过帧头前的垃圾数据 */
        if (i > 0) {
            Usart1Ring_Skip(i);
            count = Usart1Ring_GetCount();
            Usart1Ring_Peek(peek_buf, count);
        }
        
        /* 查找帧尾 "B6A5" */
        for (i = 0; i + 3 < count; i += 2) {
            if (peek_buf[i] == 'B' && peek_buf[i+1] == '6' &&
                peek_buf[i+2] == 'A' && peek_buf[i+3] == '5') {
                frame_end = i + 4;
                break;
            }
        }
        
        if (frame_end == 0) {
            /* 未找到完整帧，等待更多数据 */
            return 0;
        }
        
        /* 提取ASCII帧字符串 */
        memset(ascii_buf, 0, sizeof(ascii_buf));
        Usart1Ring_Peek((uint8_t*)ascii_buf, frame_end);
        ascii_count = frame_end;
        
        /* 开始解析 */
        g_parse_state = PARSE_STATE_HEADER_BYTE2;
        g_data_index = 0;
    }
    
    /* 状态机逐字节解析 */
    uint16_t ascii_pos = 0;
    
    switch (g_parse_state) {
        case PARSE_STATE_HEADER_BYTE2:
            g_frame_buffer.frame_header = 0xA5B6;
            g_parse_state = PARSE_STATE_DEVICE_ID_BYTE1;
            ascii_pos = 4;
            break;
            
        case PARSE_STATE_DEVICE_ID_BYTE1:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.device_id = (uint16_t)(byte_val << 8);
            g_parse_state = PARSE_STATE_DEVICE_ID_BYTE2;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_DEVICE_ID_BYTE2:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.device_id |= byte_val;
            g_parse_state = PARSE_STATE_FRAME_TYPE;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_FRAME_TYPE:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.frame_type = byte_val;
            g_parse_state = PARSE_STATE_CMD_BYTE1;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_CMD_BYTE1:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.cmd_word = (uint16_t)(byte_val << 8);
            g_parse_state = PARSE_STATE_CMD_BYTE2;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_CMD_BYTE2:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.cmd_word |= byte_val;
            g_parse_state = PARSE_STATE_DATA_LEN;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_DATA_LEN:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.data_len = byte_val;
            if (g_frame_buffer.data_len > 256) goto PARSE_ERROR;
            g_parse_state = PARSE_STATE_PROTOCOL_VER;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_PROTOCOL_VER:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.protocol_version = byte_val;
            g_data_index = 0;
            g_parse_state = PARSE_STATE_DATA;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_DATA:
            if (g_data_index < g_frame_buffer.data_len) {
                byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
                if (byte_val == 0xFF) goto PARSE_ERROR;
                g_frame_buffer.data[g_data_index++] = byte_val;
                ascii_pos += 2;
                
                if (g_data_index < g_frame_buffer.data_len) {
                    return 0;
                }
            }
            g_parse_state = PARSE_STATE_CRC_BYTE1;
            ascii_pos += (uint16_t)((g_frame_buffer.data_len - g_data_index + 1) * 2);
            break;
            
        case PARSE_STATE_CRC_BYTE1:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.crc16 = (uint16_t)(byte_val << 8);
            g_parse_state = PARSE_STATE_CRC_BYTE2;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_CRC_BYTE2:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.crc16 |= byte_val;
            g_parse_state = PARSE_STATE_TAIL_BYTE1;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_TAIL_BYTE1:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.frame_tail = (uint16_t)(byte_val << 8);
            g_parse_state = PARSE_STATE_TAIL_BYTE2;
            ascii_pos += 2;
            break;
            
        case PARSE_STATE_TAIL_BYTE2:
            byte_val = Protocol_Ascii2BytesToByte(&ascii_buf[ascii_pos]);
            if (byte_val == 0xFF) goto PARSE_ERROR;
            g_frame_buffer.frame_tail |= byte_val;
            
            /* 验证帧尾 */
            if (g_frame_buffer.frame_tail != FRAME_TAIL) {
                goto PARSE_ERROR;
            }
            
            /* 验证CRC */
            {
                uint8_t crc_buf[512];
                uint16_t crc_len = (uint16_t)(13 + g_frame_buffer.data_len);
                uint16_t crc_calc;
                
                crc_buf[0] = 0xA5;
                crc_buf[1] = 0xB6;
                crc_buf[2] = (uint8_t)((g_frame_buffer.device_id >> 8) & 0xFF);
                crc_buf[3] = (uint8_t)(g_frame_buffer.device_id & 0xFF);
                crc_buf[4] = g_frame_buffer.frame_type;
                crc_buf[5] = (uint8_t)((g_frame_buffer.cmd_word >> 8) & 0xFF);
                crc_buf[6] = (uint8_t)(g_frame_buffer.cmd_word & 0xFF);
                crc_buf[7] = g_frame_buffer.data_len;
                crc_buf[8] = g_frame_buffer.protocol_version;
                memcpy(&crc_buf[9], g_frame_buffer.data, g_frame_buffer.data_len);
                
                crc_calc = Protocol_CalcCRC16(crc_buf, crc_len);
                
                if (crc_calc != g_frame_buffer.crc16) {
                    goto PARSE_ERROR;
                }
            }
            
            /* 帧解析成功 */
            g_frame_buffer.valid = 1;
            *frame = g_frame_buffer;
            
            /* 清理接收缓冲区已处理的数据 */
            Usart1Ring_Skip(ascii_count);
            
            /* 复位状态机 */
            g_parse_state = PARSE_STATE_IDLE;
            return 1;
            
        default:
            break;
    }
    
    return 0;
    
PARSE_ERROR:
    /* 解析错误，跳过至少一个字符，重新开始 */
    Usart1Ring_Skip(2);
    g_parse_state = PARSE_STATE_IDLE;
    return 0;
}

/*======================== 协议层初始化和工具函数 ========================*/

void Protocol_Init(void)
{
    g_parse_state = PARSE_STATE_IDLE;
    g_data_index = 0;
    memset(&g_frame_buffer, 0, sizeof(g_frame_buffer));
}

uint16_t Protocol_GetRxCount(void)
{
    return Usart1Ring_GetCount();
}

void Protocol_RxReset(void)
{
    Usart1_RxReset();
    g_parse_state = PARSE_STATE_IDLE;
}

/****************************End*****************************/
