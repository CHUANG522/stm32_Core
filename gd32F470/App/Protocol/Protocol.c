#include "Protocol.h"
#include "USART1.h"

static uint8_t AsciiCharToHex(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static uint16_t AsciiToBytes(const uint8_t *src, uint16_t src_len, uint8_t *dst)
{
    if (src_len % 2 != 0) return 0;
    uint16_t dst_len = src_len / 2;
    for (uint16_t i = 0; i < dst_len; i++) {
        dst[i] = (AsciiCharToHex(src[i * 2]) << 4) | AsciiCharToHex(src[i * 2 + 1]);
    }
    return dst_len;
}

static void BytesToAscii(const uint8_t *src, uint16_t src_len, char *dst)
{
    const char hex[] = "0123456789ABCDEF";
    for (uint16_t i = 0; i < src_len; i++) {
        dst[i * 2]     = hex[src[i] >> 4];
        dst[i * 2 + 1] = hex[src[i] & 0x0F];
    }
    dst[src_len * 2] = '\0';
}

uint16_t CRC16_Modbus(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool Protocol_ParseFrame(ProtocolFrame_t *frame)
{
    if (frame == NULL) return false;

    /* 1. 寻找帧头 A5B6 的 ASCII 表示 */
    while (Usart1Ring_GetCount() >= 26) {
        uint8_t peek[4];
        Usart1Ring_Peek(peek, 4);
        if (peek[0] == 'A' && peek[1] == '5' && peek[2] == 'B' && peek[3] == '6') {
            break;
        }
        Usart1Ring_Skip(1);
    }
    if (Usart1Ring_GetCount() < 26) return false;

    /* 2. 读取 ASCII 长度字段 */
    uint8_t len_ascii[2];
    uint16_t temp_tail = g_usart1_ring.tail;
    temp_tail = (temp_tail + 14) % USART1_RX_BUF_SIZE;
    len_ascii[0] = g_usart1_ring.buf[temp_tail];
    temp_tail = (temp_tail + 1) % USART1_RX_BUF_SIZE;
    len_ascii[1] = g_usart1_ring.buf[temp_tail];
    uint8_t content_len = (AsciiCharToHex(len_ascii[0]) << 4) | AsciiCharToHex(len_ascii[1]);

    /* 3. content_len 越界检查 → 长度错误 */
    if (content_len > 128) {
        Protocol_SendErrorFrame(0xFFFF);
        Usart1Ring_Skip(4);
        return false;
    }

    uint16_t total_ascii_len = 26 + ((uint16_t)content_len * 2);

    /* 4. 总帧长越界检查 → 长度错误 */
    if (total_ascii_len > 512) {
        Protocol_SendErrorFrame(0xFFFF);
        Usart1Ring_Skip(4);
        return false;
    }

    if (Usart1Ring_GetCount() < total_ascii_len) return false;

    uint8_t ascii_frame[512];
    Usart1Ring_Peek(ascii_frame, total_ascii_len);

    /* 5. 校验帧尾 B6A5 → 长度/格式错误 */
    uint16_t tail_idx = total_ascii_len - 4;
    if (!(ascii_frame[tail_idx]     == 'B' &&
          ascii_frame[tail_idx + 1] == '6' &&
          ascii_frame[tail_idx + 2] == 'A' &&
          ascii_frame[tail_idx + 3] == '5')) {
        Protocol_SendErrorFrame(0xFFFF);
        Usart1Ring_Skip(4);
        return false;
    }

    /* 6. ASCII 转二进制 */
    uint8_t bin_frame[300];
    uint16_t bin_len = AsciiToBytes(ascii_frame, total_ascii_len, bin_frame);

    /* 7. 二进制长度校验 → 长度错误（此时可提取 device_id） */
    if (bin_len != (13 + content_len)) {
        uint16_t err_device_id = ((uint16_t)bin_frame[2] << 8) | bin_frame[3];
        Protocol_SendErrorFrame(err_device_id);
        Usart1Ring_Skip(4);
        return false;
    }

    /* 8. CRC 校验 */
    uint16_t crc_calc_len = 9 + content_len;
    uint16_t crc_received = ((uint16_t)bin_frame[crc_calc_len] << 8) | bin_frame[crc_calc_len + 1];
    uint16_t crc_calculated = CRC16_Modbus(bin_frame, crc_calc_len);

    if (crc_received != crc_calculated) {
        uint16_t err_device_id = ((uint16_t)bin_frame[2] << 8) | bin_frame[3];
        Protocol_SendErrorFrame(err_device_id);
        Usart1Ring_Skip(4);
        return false;
    }

    /* 9. 填充帧结构体 */
    frame->device_id  = ((uint16_t)bin_frame[2] << 8) | bin_frame[3];
    frame->frame_type = bin_frame[4];
    frame->cmd_word   = ((uint16_t)bin_frame[5] << 8) | bin_frame[6];
    frame->data_len   = bin_frame[7];
    if (frame->data_len > 0 && frame->data_len <= 128) {
        memcpy(frame->data, &bin_frame[9], frame->data_len);
    }
    frame->crc16 = crc_received;
    Usart1Ring_Skip(total_ascii_len);
    return true;
}

void Protocol_SendFrame(uint16_t device_id, uint8_t frame_type, uint16_t cmd_word,
                               const uint8_t *data, uint8_t data_len)
{
    uint8_t bin_frame[300];
    uint16_t pos = 0;
    bin_frame[pos++] = 0xA5;
    bin_frame[pos++] = 0xB6;
    bin_frame[pos++] = (device_id >> 8) & 0xFF;
    bin_frame[pos++] = device_id & 0xFF;
    bin_frame[pos++] = frame_type;
    bin_frame[pos++] = (cmd_word >> 8) & 0xFF;
    bin_frame[pos++] = cmd_word & 0xFF;
    bin_frame[pos++] = data_len;
    bin_frame[pos++] = PROTOCOL_VERSION;
    if (data_len > 0) {
        memcpy(&bin_frame[pos], data, data_len);
        pos += data_len;
    }
    uint16_t crc = CRC16_Modbus(bin_frame, pos);
    bin_frame[pos++] = (crc >> 8) & 0xFF;
    bin_frame[pos++] = crc & 0xFF;
    bin_frame[pos++] = 0xB6;
    bin_frame[pos++] = 0xA5;
    char ascii_frame[600];
    BytesToAscii(bin_frame, pos, ascii_frame);
    RS485_SendData((uint8_t*)ascii_frame, strlen(ascii_frame));
}

void Protocol_SendAck(uint16_t device_id, uint16_t cmd_word,
                      const uint8_t *data, uint8_t data_len)
{
    Protocol_SendFrame(device_id, FRAME_TYPE_ACK, cmd_word, data, data_len);
}

void Protocol_SendError(uint16_t device_id)
{
    uint8_t empty = 0x00;
    Protocol_SendFrame(device_id, FRAME_TYPE_ERROR, CMD_ERROR_GENERAL, &empty, 0);
}

void Protocol_SendHeartbeat(uint16_t device_id)
{
    uint8_t empty = 0x00;
    Protocol_SendFrame(device_id, FRAME_TYPE_HEART, CMD_HEARTBEAT, &empty, 0);
}
