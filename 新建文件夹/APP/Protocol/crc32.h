/************************************************************
 * 文件：crc32.h
 * 说明：CRC32 计算接口
************************************************************/

#ifndef __CRC32_H__
#define __CRC32_H__

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************ 函数定义 ************************/

uint32_t crc32_calc(uint8_t* data , uint32_t len);
uint32_t crc32_init(void);
uint32_t crc32_update(uint32_t crc , const uint8_t* data , uint32_t len);
uint32_t crc32_finalize(uint32_t crc);

#endif

/****************************End*****************************/
