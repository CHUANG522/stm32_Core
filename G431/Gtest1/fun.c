#include <HeadFile.h>

void led_show(uint8_t led,uint8_t mode )
{
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	
	uint16_t GPIOC_PIN [] ={GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};
	uint8_t PIN ;	
	PIN =GPIOC_PIN[led - 8];
	if(mode ==1)
	HAL_GPIO_WritePin(GPIOC,PIN,GPIO_PIN_RESET);
	else 
	HAL_GPIO_WritePin(GPIOC,PIN,GPIO_PIN_SET);
	
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
	
}

char text[20];
void lcd_show(void)
{
   sprintf(text,  " text ");
	LCD_DisplayStringLine(Line0,(uint8_t *)text);

}
