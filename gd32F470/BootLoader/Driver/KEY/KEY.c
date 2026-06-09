/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：key.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/

/************************* 头文件 *************************/

#include "KEY.h"

/************************ 全局变量定义 ************************/
KEY keys[4];

/************************************************************ 
 * Function :       KEY_Init
 * Comment  :       用于初始化LED端口
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void KEY_Init(void)
{
	rcu_periph_clock_enable(RCU_SYSCFG);    // 初始化SYSCFG时钟	
    rcu_periph_clock_enable(KEY_CLK);								// 初始化KEY总线时钟
    
    gpio_mode_set(KEY_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                 KEY1_PIN | KEY2_PIN | KEY3_PIN | KEY4_PIN);		//配置GPIO模式为输入
}


/************************************************************ 
 * Function :       KEY_Scan
 * Comment  :       void
 * Parameter:       void
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/
void KEY_Scan(void)
{
	keys[0].pin_state = gpio_input_bit_get(KEY_PORT,KEY1_PIN) == 0?1:0;
	keys[1].pin_state = gpio_input_bit_get(KEY_PORT,KEY2_PIN) == 0?1:0;
	keys[2].pin_state = gpio_input_bit_get(KEY_PORT,KEY3_PIN) == 0?1:0;
	keys[3].pin_state = gpio_input_bit_get(KEY_PORT,KEY4_PIN) == 0?1:0;
	
	for(int i=0;i<4;i++)
	{
		switch(keys[i].state)
		{
			case 0:
				if(keys[i].pin_state)
				{
					keys[i].press_count++;
				}
				if(!keys[i].pin_state)
				{
					if(keys[i].press_count>=2)
					{
						keys[i].state = 1;
					}
					else
					{
						keys[i].press_count = 0;
					}
				}
				break;
			
			case 1:
				if(!keys[i].pin_state)
				{
					keys[i].release_count++;
				}
				if(keys[i].release_count>=2)
				{
					keys[i].state = 2;
				}
				break;
			
			case 2:
				keys[i].short_press = 1;
				if(keys[i].press_count>=200)
				{
					keys[i].long_press = 1;
					keys[i].short_press = 0;
				}
				keys[i].press_count = 0;
				keys[i].release_count = 0;
				break;
			
			default:
				keys[i].state = 0;
				break;
		}
	}
	
}







/****************************End*****************************/

