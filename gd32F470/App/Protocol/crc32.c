/************************************************************
 * 文件：crc32.c
 * 说明：CRC32 计算实现
************************************************************/

/************************* 头文件 *************************/

#include "crc32.h"

/************************ 函数定义 ************************/

uint32_t crc32_calc(uint8_t* data , uint32_t len)
{
	return crc32_finalize(crc32_update(crc32_init() , data , len));
}

uint32_t crc32_init(void)
{
	return 0xFFFFFFFF;
}

uint32_t crc32_update(uint32_t crc , const uint8_t* data , uint32_t len)
{
	uint32_t i , j;

	for (i = 0; i < len; i++)
	{
		crc ^= data[i];
		for (j = 0; j < 8; j++)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		}
	}

	return crc;
}

uint32_t crc32_finalize(uint32_t crc)
{
	return crc ^ 0xFFFFFFFF;
}

/****************************End*****************************/
