#ifndef __FUNCTION_H
#define __FUNCTION_H

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/


/************************ 函数定义 ************************/

void System_Init(void);      	// 系统初始化
void UsrFunction(void);         // 用户函数
void Init_LED_Stat(void);		// 系统初始化时用LED显示状态
void mcu_software_reset(void);
void APP_HandleUpgradeRequest(void);


void spi_flash_write_version(uint32_t addr);
void spi_flash_read_version(uint32_t addr, uint8_t ver[4]);
#endif


/****************************End*****************************/

