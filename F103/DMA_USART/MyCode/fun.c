#include "fun.h"
void function(void)
{
	OLED_show();
}
uint8_t OLED_buff[20];
uint32_t time_OLED;
uint32_t time;
void OLED_show(void)
{
	if (time_OLED < 1000)
		return; // 1秒刷新
	time_OLED = 0;
	send_string((uint8_t *)"hello\r\n");
	OLED_Clear();
	sprintf((char *)OLED_buff, "time_OLED:%d", time_OLED);
	OLED_ShowString(1, 1, (char *)OLED_buff);
}
