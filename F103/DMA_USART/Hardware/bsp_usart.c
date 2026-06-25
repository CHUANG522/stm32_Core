#include "bsp_usart.h"
#define string_max_len 256
void send_string(uint8_t *string)
{
    if (string == NULL)
        return;
    uint16_t string_len = strlen((char *)string);
    if (string_len > string_max_len)
        string_len = string_max_len;

    uint16_t time_out = (string_len + 1) * 2;
    HAL_UART_Transmit(&huart1, string, string_len, time_out);
}
