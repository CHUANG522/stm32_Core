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
	if(time_OLED%1000 >100) return ;//0.1秒刷新
	
	 OLED_Clear();
	sprintf((char *)OLED_buff,"time:%d",time);
 OLED_ShowString(1,1,(char *)OLED_buff);
	


}
