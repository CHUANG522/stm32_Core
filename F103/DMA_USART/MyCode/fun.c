#include "fun.h"
void function(void)
{
	OLED_show();
	USART_show();
}
uint8_t OLED_buff[20];
uint32_t time_OLED;

void OLED_show(void)
{
	if (time_OLED < 1000)
		return; // 1秒刷新
	time_OLED = 0;
	OLED_Clear();
	sprintf((char *)OLED_buff, "time_OLED:%d", time_OLED);
	OLED_ShowString(1, 1, (char *)OLED_buff);
}
uint32_t time_USART;
uint8_t USART_buff[32];
void USART_show(void)
{
	if (time_USART < 1000)
		return; // 1秒发送一次数据
	time_USART = 0;

	sprintf((char *)USART_buff, "hello\r\n");
	send_string(USART_buff);
}
