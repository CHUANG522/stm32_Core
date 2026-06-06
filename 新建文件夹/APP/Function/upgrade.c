///************************************************************
// * 文件：upgrade.c
// * 说明：串口 App 升级任务
//************************************************************/

///************************* 头文件 *************************/

//#include "upgrade.h"


///************************ 变量定义 ************************/

//typedef struct __attribute__((packed)) Parameter_SUM
//{
//	BootParam_t BootParam;
//	BootParam_t BootParam_Reserved;
//	UpdateLog_t UpdateLog;
//	UserConfig_t UserConfig;
//	CalibData_t CalibData;

//}Parameter_t;

//static Parameter_t my_param_sum = { 0 };

//static uint8_t config_buf[BOOT_PARAM_SIZE] = { 0 };
//static uint8_t header_buf[UPDATE_HEADER_SIZE] = { 0 };
//static uint8_t write_buf[UPGRADE_WRITE_CHUNK_SIZE] = { 0 };

//typedef enum
//{
//	UPGRADE_WAIT_HEADER = 0,
//	UPGRADE_RECEIVE_PAYLOAD,
//	UPGRADE_ERROR
//}UpgradeState_t;

//typedef struct
//{
//	uint32_t magicWord;
//	uint32_t headerVersion;
//	uint32_t appVersion;
//	uint32_t appSize;
//	uint32_t appCRC32;
//	uint32_t headerSize;
//	uint32_t reserved;
//	uint32_t headerCRC32;
//}UpgradeHeader_t;

//static UpgradeState_t upgrade_state = UPGRADE_WAIT_HEADER;
//static UpgradeHeader_t upgrade_header = { 0 };
//static uint32_t header_rx_len = 0;
//static uint32_t payload_rx_len = 0;
//static uint32_t write_buf_len = 0;
//static uint32_t payload_crc_state = 0;

///************************ 函数定义 ************************/

//static void Upgrade_ResetState(void);
//static uint32_t Upgrade_ReadU32BE(const uint8_t* data);
//static uint8_t Upgrade_ParseHeader(void);
//static uint8_t Upgrade_CheckHeader(void);
//static uint8_t Upgrade_CheckBlank(uint32_t addr , uint32_t len);
//static void Upgrade_EraseDownloadAreaOnBoot(void);
//static uint8_t Upgrade_WritePayload(const uint8_t* data , uint32_t len);
//static uint8_t Upgrade_FlushPayload(void);
//static void Upgrade_HandleByte(uint8_t byte);
//static void Upgrade_FinishPayload(void);
//static void Upgrade_LoadBootParam(void);
//static void Upgrade_SaveBootParam(void);
//static void Upgrade_UpdateBootParam(uint32_t appVersion , uint32_t payload_size , uint32_t payload_crc , uint32_t current_app_crc);

///**
// * 初始化升级模块的默认参数。
// */
//void Upgrade_Init(void)
//{
//	bootloader_config_init(&my_param_sum.BootParam , &my_param_sum.UpdateLog , &my_param_sum.UserConfig , &my_param_sum.CalibData);
//	Upgrade_ResetState();
//	Usart1_RxReset();
//#if APP_ERASE_DOWNLOAD_ON_BOOT
//	Upgrade_EraseDownloadAreaOnBoot();
//	Upgrade_ResetState();
//	Uart_RxReset();
//#endif
//	RS485_Printf("Upgrade : ready, header size:%d\r\n" , UPDATE_HEADER_SIZE);
//}

///**
// * @brief 
// * 
// *
// *
// * 升级包格式:32 字节 header + App payload.
// */
//void Upgrade_Task(void)
//{
//	uint8_t byte = 0;

//	if (Usart1_RxOverflow() != 0)
//	{
//		RS485_Printf("Upgrade : uart ring overflow\r\n");
//		Upgrade_ResetState();
//		Usart1_RxReset();
//		return;
//	}

//	while (Usart1_ReadByte(&byte) != 0)
//	{
//		
//		Upgrade_HandleByte(byte);
//	}
//}

///**
// * 复位串口接收状态。
// */
//static void Upgrade_ResetState(void)
//{
//	upgrade_state = UPGRADE_WAIT_HEADER;
//	memset(header_buf , 0 , sizeof(header_buf));
//	memset(write_buf , 0xFF , sizeof(write_buf));
//	memset(&upgrade_header , 0 , sizeof(upgrade_header));
//	header_rx_len = 0;
//	payload_rx_len = 0;
//	write_buf_len = 0;
//	payload_crc_state = crc32_init();
//}

///**
// * 按大端字节序读取 4 字节无符号整数。
// *
// * 升级包头中的 magicWord 和 appVersion 均按大端顺序发送。
// */
//static uint32_t Upgrade_ReadU32BE(const uint8_t* data)
//{
//	return ((uint32_t)data[0] << 24) |
//	       ((uint32_t)data[1] << 16) |
//	       ((uint32_t)data[2] << 8) |
//	       ((uint32_t)data[3]);
//}

///**
// * 解析串口升级包头。
// *
// * Args:
// *   magic: 输出 magicWord。
// *   appVersion: 输出 App 版本号。
// * Returns:
// *   1 表示成功，0 表示失败。
// */
//static uint8_t Upgrade_ParseHeader(void)
//{
//	upgrade_header.magicWord = Upgrade_ReadU32BE(&header_buf[0]);
//	upgrade_header.headerVersion = Upgrade_ReadU32BE(&header_buf[4]);
//	upgrade_header.appVersion = Upgrade_ReadU32BE(&header_buf[8]);
//	upgrade_header.appSize = Upgrade_ReadU32BE(&header_buf[12]);
//	upgrade_header.appCRC32 = Upgrade_ReadU32BE(&header_buf[16]);
//	upgrade_header.headerSize = Upgrade_ReadU32BE(&header_buf[20]);
//	upgrade_header.reserved = Upgrade_ReadU32BE(&header_buf[24]);
//	upgrade_header.headerCRC32 = Upgrade_ReadU32BE(&header_buf[28]);
//	return 1;
//}

///**
// * 检查升级包合法性。
// *
// * Args:
// *   magic: 升级包头中的 magicWord。
// *   payload_size: App payload 长度。
// * Returns:
// *   1 表示通过，0 表示失败。
// */
//static uint8_t Upgrade_CheckHeader(void)
//{
//	uint32_t header_crc32 = 0;

//	if (upgrade_header.magicWord != APP_MAGIC_WORD)
//	{
//		RS485_Printf("Upgrade : magic error 0x%08X\r\n" , upgrade_header.magicWord);
//		return 0;
//	}

//	if (upgrade_header.headerVersion != UPDATE_HEADER_VERSION)
//	{
//		RS485_Printf("Upgrade : header version error 0x%08X\r\n" , upgrade_header.headerVersion);
//		return 0;
//	}

//	if (upgrade_header.headerSize != UPDATE_HEADER_SIZE)
//	{
//		RS485_Printf("Upgrade : header size error %d\r\n" , upgrade_header.headerSize);
//		return 0;
//	}

//	header_crc32 = crc32_calc(header_buf , UPDATE_HEADER_CRC_OFFSET);
//	if (header_crc32 != upgrade_header.headerCRC32)
//	{
//		RS485_Printf("Upgrade : header crc error 0x%08X != 0x%08X\r\n" , header_crc32 , upgrade_header.headerCRC32);
//		return 0;
//	}

//	if (upgrade_header.appSize == 0)
//	{
//		RS485_Printf("Upgrade : app size is zero\r\n");
//		return 0;
//	}

//	if (upgrade_header.appSize > APP_MAX_SIZE)
//	{
//		RS485_Printf("Upgrade : app size over APP_MAX_SIZE: %d\r\n" , upgrade_header.appSize);
//		return 0;
//	}

//	if (upgrade_header.appSize > DOWNLOAD_AREA_SIZE)
//	{
//		RS485_Printf("Upgrade : app size over DOWNLOAD_AREA_SIZE: %d\r\n" , upgrade_header.appSize);
//		return 0;
//	}

//	return 1;
//}

//static uint8_t Upgrade_CheckBlank(uint32_t addr , uint32_t len)
//{
//	for (uint32_t i = 0; i < len; i++)
//	{
//		if (internal_flash_read_Char(addr + i) != 0xFF)
//		{
//			RS485_Printf("Upgrade : download area not blank at 0x%08X\r\n" , addr + i);
//			return 0;
//		}
//	}
//	return 1;
//}

//static void Upgrade_EraseDownloadAreaOnBoot(void)
//{
//	RS485_Printf("Upgrade : erase download area on boot\r\n");
//	RS485_Printf("Upgrade : erase download pages:%d\r\n" , DOWNLOAD_AREA_PAGE_COUNT);
//	for (uint32_t i = 0; i < DOWNLOAD_AREA_PAGE_COUNT; i++)
//	{
//		internal_flash_erase(DOWNLOAD_AREA_ADDR + i * FLASH_PAGE_SIZE);
//	}
//}

//static uint8_t Upgrade_WritePayload(const uint8_t* data , uint32_t len)
//{
//	if (len == 0)
//	{
//		return 1;
//	}

//	if (Upgrade_CheckBlank(DOWNLOAD_AREA_ADDR + payload_rx_len , len) == 0)
//	{
//		return 0;
//	}

//	internal_flash_write_str_Char(DOWNLOAD_AREA_ADDR + payload_rx_len , (uint8_t*)data , len);
//	payload_crc_state = crc32_update(payload_crc_state , data , len);
//	payload_rx_len += len;
//	return 1;
//}

//static uint8_t Upgrade_FlushPayload(void)
//{
//	uint32_t write_len = write_buf_len;

//	if (write_len == 0)
//	{
//		return 1;
//	}

//	if ((payload_rx_len + write_len) > upgrade_header.appSize)
//	{
//		RS485_Printf("Upgrade : payload overrun\r\n");
//		return 0;
//	}

//	if (Upgrade_WritePayload(write_buf , write_len) == 0)
//	{
//		return 0;
//	}

//	write_buf_len = 0;
//	return 1;
//}

//static void Upgrade_HandleByte(uint8_t byte)
//{
//	if (upgrade_state == UPGRADE_ERROR)
//	{
//		return;
//	}

//	if (upgrade_state == UPGRADE_WAIT_HEADER)
//	{
//		header_buf[header_rx_len++] = byte;
//		if (header_rx_len < UPDATE_HEADER_SIZE)
//		{
//			return;
//		}

//		Upgrade_ParseHeader();
//		if (Upgrade_CheckHeader() == 0)
//		{
//			upgrade_state = UPGRADE_ERROR;
//			RS485_Printf("Upgrade : header check fail\r\n");
//			return;
//		}

//		RS485_Printf("Upgrade : header ok\r\n");
//		RS485_Printf("Upgrade : appVersion:0x%08X\r\n" , upgrade_header.appVersion);
//		RS485_Printf("Upgrade : appSize:%d\r\n" , upgrade_header.appSize);
//		RS485_Printf("Upgrade : appCRC32:0x%08X\r\n" , upgrade_header.appCRC32);
////		printf("Upgrade : header ok\r\n");
////		printf("Upgrade : magicWord:0x%08X\r\n" , upgrade_header.magicWord);
////		printf("Upgrade : headerVersion:0x%08X\r\n" , upgrade_header.headerVersion);
////		printf("Upgrade : appVersion:0x%08X\r\n" , upgrade_header.appVersion);
////		printf("Upgrade : appSize:%d\r\n" , upgrade_header.appSize);
////		printf("Upgrade : appCRC32:0x%08X\r\n" , upgrade_header.appCRC32);
////		printf("Upgrade : headerSize:0x%08X\r\n" , upgrade_header.headerSize);
////		printf("Upgrade : reserved:0x%08X\r\n" , upgrade_header.reserved);
////		printf("Upgrade : headerCRC32:0x%08X\r\n" , upgrade_header.headerCRC32);
//		upgrade_state = UPGRADE_RECEIVE_PAYLOAD;
//		return;
//	}

//	if (upgrade_state == UPGRADE_RECEIVE_PAYLOAD)
//	{
//		write_buf[write_buf_len++] = byte;
//		if (write_buf_len >= UPGRADE_WRITE_CHUNK_SIZE)
//		{
//			if (Upgrade_FlushPayload() == 0)
//			{
//				upgrade_state = UPGRADE_ERROR;
//				return;
//			}
//		}

//		if ((payload_rx_len + write_buf_len) >= upgrade_header.appSize)
//		{
//			if (Upgrade_FlushPayload() == 0)
//			{
//				upgrade_state = UPGRADE_ERROR;
//				return;
//			}
//			Upgrade_FinishPayload();
//		}
//	}
//}

//static void Upgrade_FinishPayload(void)
//{
//	uint32_t payload_crc32 = crc32_finalize(payload_crc_state);
//	uint32_t app_crc32 = 0;

//	if (payload_rx_len != upgrade_header.appSize)
//	{
//		RS485_Printf("Upgrade : app size mismatch %d/%d\r\n" , payload_rx_len , upgrade_header.appSize);
//		upgrade_state = UPGRADE_ERROR;
//		return;
//	}

//	RS485_Printf("Upgrade : received %d bytes\r\n" , payload_rx_len);
//	RS485_Printf("Upgrade : payload CRC32:0x%08X\r\n" , payload_crc32);

//	if (payload_crc32 != upgrade_header.appCRC32)
//	{
//		RS485_Printf("Upgrade : payload crc check fail\r\n");
//		upgrade_state = UPGRADE_ERROR;
//		return;
//	}

//	RS485_Printf("Upgrade : payload crc check pass\r\n");

//	app_crc32 = crc32_calc((uint8_t*)APP_START_ADDR , APP_MAX_SIZE);
//	RS485_Printf("Upgrade : current app crc32:0x%08X\r\n" , app_crc32);

//	Upgrade_UpdateBootParam(upgrade_header.appVersion , upgrade_header.appSize , upgrade_header.appCRC32 , app_crc32);
//	Upgrade_SaveBootParam();
//	RS485_Printf("Upgrade : write BootParam done\r\n");
//	
//	mcu_software_reset();
//}

///**
// * 读取参数页。
// */
//static void Upgrade_LoadBootParam(void)
//{
//	for (uint16_t i = 0; i < BOOT_PARAM_SIZE; i++)
//	{
//		config_buf[i] = internal_flash_read_Char(BOOT_PARAM_ADDR + i);
//	}

//	memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));
//}

///**
// * 写回参数页。
// */
//static void Upgrade_SaveBootParam(void)
//{
//	internal_flash_erase(BOOT_PARAM_ADDR);
//	memcpy(config_buf , &my_param_sum , sizeof(Parameter_t));
//	internal_flash_write_str_Char(BOOT_PARAM_ADDR , config_buf , sizeof(Parameter_t));
//}

///**
// * 更新 BootParam 中的升级请求字段。
// *
// * Args:
// *   appVersion: 新 App 版本号。
// *   payload_size: 新 App payload 长度。
// *   payload_crc: 新 App payload CRC32。
// *   current_app_crc: 当前 App 分区 CRC32。
// */
//static void Upgrade_UpdateBootParam(uint32_t appVersion , uint32_t payload_size , uint32_t payload_crc , uint32_t current_app_crc)
//{
//	Upgrade_LoadBootParam();

//	/* 保留旧参数，BootLoader 用它判断是否需要先备份当前 App。 */
//	my_param_sum.BootParam_Reserved = my_param_sum.BootParam;

//	my_param_sum.BootParam.appStartAddr = APP_START_ADDR;
//	my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);	//栈顶地址
//	my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);	// 应用程序入口地址在应用程序起始地址后4字节(需要手动指定中断向量表的位置)
//	my_param_sum.BootParam.magicWord = APP_MAGIC_WORD;
//	my_param_sum.BootParam.appVersion = appVersion;
//	my_param_sum.BootParam.backupCRC32 = current_app_crc;
//	RS485_Printf("my_param_sum.BootParam.backupCRC32 : 0x%08X\r\n" , my_param_sum.BootParam.backupCRC32);

//	my_param_sum.BootParam.updateFlag = UPDATE_REQUEST_FLAG;
//	my_param_sum.BootParam.updateStatus = UPDATE_STATUS_WAIT_BOOTLOADER;
//	my_param_sum.BootParam.appSize = payload_size;
//	my_param_sum.BootParam.appCRC32 = payload_crc;
//}

///****************************End*****************************/


/************************************************************
 * 文件：upgrade.c
 * 说明：串口 App 升级任务
 * 修改：header 简化为 4 字节魔术字 0x5AA5C33C，后面直接为固件数据
************************************************************/

/************************* 头文件 *************************/

#include "upgrade.h"

/************************ 新增宏定义 ************************/

#define UPGRADE_PACKET_MAGIC    0x5AA5C33C
#define UPGRADE_MAGIC_SIZE      4
#define UPGRADE_IDLE_MAX        2000    /* 串口空闲阈值，假设主循环 1 ms/次，约 2 s */

/************************ 变量定义 ************************/

typedef struct __attribute__((packed)) Parameter_SUM
{
	BootParam_t BootParam;
	BootParam_t BootParam_Reserved;
	UpdateLog_t UpdateLog;
	UserConfig_t UserConfig;
	CalibData_t CalibData;

}Parameter_t;

static Parameter_t my_param_sum = { 0 };

static uint8_t config_buf[BOOT_PARAM_SIZE] = { 0 };
static uint8_t header_buf[UPGRADE_MAGIC_SIZE] = { 0 };
static uint8_t write_buf[UPGRADE_WRITE_CHUNK_SIZE] = { 0 };

typedef enum
{
	UPGRADE_WAIT_HEADER = 0,
	UPGRADE_RECEIVE_PAYLOAD,
	UPGRADE_ERROR
}UpgradeState_t;

/* 简化后的包头：仅保留魔术字 */
typedef struct
{
	uint32_t magicWord;
}UpgradeHeader_t;

static UpgradeState_t upgrade_state = UPGRADE_WAIT_HEADER;
static UpgradeHeader_t upgrade_header = { 0 };
static uint32_t header_rx_len = 0;
static uint32_t payload_rx_len = 0;
static uint32_t write_buf_len = 0;
static uint32_t payload_crc_state = 0;
static uint32_t upgrade_idle_cnt = 0;

/************************ 函数定义 ************************/

static void Upgrade_ResetState(void);
static uint32_t Upgrade_ReadU32BE(const uint8_t* data);
static uint8_t Upgrade_ParseHeader(void);
static uint8_t Upgrade_CheckHeader(void);
static uint8_t Upgrade_CheckBlank(uint32_t addr , uint32_t len);
static void Upgrade_EraseDownloadAreaOnBoot(void);
static uint8_t Upgrade_WritePayload(const uint8_t* data , uint32_t len);
static uint8_t Upgrade_FlushPayload(void);
static void Upgrade_HandleByte(uint8_t byte);
static void Upgrade_FinishPayload(void);
static void Upgrade_LoadBootParam(void);
static void Upgrade_SaveBootParam(void);
static void Upgrade_UpdateBootParam(uint32_t appVersion , uint32_t payload_size , uint32_t payload_crc , uint32_t current_app_crc);

/**
 * 初始化升级模块的默认参数。
 */
void Upgrade_Init(void)
{
	bootloader_config_init(&my_param_sum.BootParam , &my_param_sum.UpdateLog , &my_param_sum.UserConfig , &my_param_sum.CalibData);
	Upgrade_ResetState();
	Usart1_RxReset();
#if APP_ERASE_DOWNLOAD_ON_BOOT
	Upgrade_EraseDownloadAreaOnBoot();
	Upgrade_ResetState();
	Uart_RxReset();
#endif
	RS485_Printf("Upgrade : ready, magic:0x%08X\r\n" , UPGRADE_PACKET_MAGIC);
}

/**
 * @brief 升级任务
 * 
 * 升级包格式: 4 字节 magic (0x5AA5C33C) + App payload.
 */
void Upgrade_Task(void)
{
	uint8_t byte = 0;
	uint8_t received = 0;

	if (Usart1_RxOverflow() != 0)
	{
		printf("Upgrade : uart ring overflow\r\n");
		Upgrade_ResetState();
		Usart1_RxReset();
		return;
	}

	while (Usart1_ReadByte(&byte) != 0)
	{
		received = 1;
		Upgrade_HandleByte(byte);
	}

	/* 串口空闲检测：连续多次无数据且已接收 payload，认为传输结束 */
	if (!received && upgrade_state == UPGRADE_RECEIVE_PAYLOAD
	    && (payload_rx_len > 0 || write_buf_len > 0))
	{
		upgrade_idle_cnt++;
		if (upgrade_idle_cnt >= UPGRADE_IDLE_MAX)
		{
			if (Upgrade_FlushPayload() == 0)
			{
				upgrade_state = UPGRADE_ERROR;
				return;
			}
			Upgrade_FinishPayload();
			upgrade_idle_cnt = 0;
		}
	}
	else
	{
		upgrade_idle_cnt = 0;
	}
}

/**
 * 复位串口接收状态。
 */
static void Upgrade_ResetState(void)
{
	upgrade_state = UPGRADE_WAIT_HEADER;
	memset(header_buf , 0 , sizeof(header_buf));
	memset(write_buf , 0xFF , sizeof(write_buf));
	memset(&upgrade_header , 0 , sizeof(upgrade_header));
	header_rx_len = 0;
	payload_rx_len = 0;
	write_buf_len = 0;
	payload_crc_state = crc32_init();
	upgrade_idle_cnt = 0;
}

/**
 * 按大端字节序读取 4 字节无符号整数。
 */
static uint32_t Upgrade_ReadU32BE(const uint8_t* data)
{
	return ((uint32_t)data[0] << 24) |
	       ((uint32_t)data[1] << 16) |
	       ((uint32_t)data[2] << 8) |
	       ((uint32_t)data[3]);
}

/**
 * 解析串口升级包头（仅魔术字）。
 */
static uint8_t Upgrade_ParseHeader(void)
{
	upgrade_header.magicWord = Upgrade_ReadU32BE(&header_buf[0]);
	return 1;
}

/**
 * 检查升级包合法性（仅检查魔术字）。
 */
static uint8_t Upgrade_CheckHeader(void)
{
	if (upgrade_header.magicWord != UPGRADE_PACKET_MAGIC)
	{
		printf("Upgrade : magic error 0x%08X\r\n" , upgrade_header.magicWord);
		return 0;
	}
	return 1;
}

static uint8_t Upgrade_CheckBlank(uint32_t addr , uint32_t len)
{
	for (uint32_t i = 0; i < len; i++)
	{
		if (internal_flash_read_Char(addr + i) != 0xFF)
		{
			printf("Upgrade : download area not blank at 0x%08X\r\n" , addr + i);
			return 0;
		}
	}
	return 1;
}

static void Upgrade_EraseDownloadAreaOnBoot(void)
{
	printf("Upgrade : erase download area on boot\r\n");
	printf("Upgrade : erase download pages:%d\r\n" , DOWNLOAD_AREA_PAGE_COUNT);
	for (uint32_t i = 0; i < DOWNLOAD_AREA_PAGE_COUNT; i++)
	{
		internal_flash_erase(DOWNLOAD_AREA_ADDR + i * FLASH_PAGE_SIZE);
	}
}

static uint8_t Upgrade_WritePayload(const uint8_t* data , uint32_t len)
{
	if (len == 0)
	{
		return 1;
	}

	if (Upgrade_CheckBlank(DOWNLOAD_AREA_ADDR + payload_rx_len , len) == 0)
	{
		return 0;
	}

	internal_flash_write_str_Char(DOWNLOAD_AREA_ADDR + payload_rx_len , (uint8_t*)data , len);
	payload_crc_state = crc32_update(payload_crc_state , data , len);
	payload_rx_len += len;
	return 1;
}

static uint8_t Upgrade_FlushPayload(void)
{
	uint32_t write_len = write_buf_len;

	if (write_len == 0)
	{
		return 1;
	}

	if (Upgrade_WritePayload(write_buf , write_len) == 0)
	{
		return 0;
	}

	write_buf_len = 0;
	return 1;
}

static void Upgrade_HandleByte(uint8_t byte)
{
	if (upgrade_state == UPGRADE_ERROR)
	{
		return;
	}

	if (upgrade_state == UPGRADE_WAIT_HEADER)
	{
		header_buf[header_rx_len++] = byte;
		if (header_rx_len < UPGRADE_MAGIC_SIZE)
		{
			return;
		}

		Upgrade_ParseHeader();
		if (Upgrade_CheckHeader() == 0)
		{
			upgrade_state = UPGRADE_ERROR;
			printf("Upgrade : header check fail\r\n");
			return;
		}

		printf("Upgrade : magic ok 0x%08X\r\n" , upgrade_header.magicWord);
		upgrade_state = UPGRADE_RECEIVE_PAYLOAD;
		return;
	}

	if (upgrade_state == UPGRADE_RECEIVE_PAYLOAD)
	{
		write_buf[write_buf_len++] = byte;
		if (write_buf_len >= UPGRADE_WRITE_CHUNK_SIZE)
		{
			if (Upgrade_FlushPayload() == 0)
			{
				upgrade_state = UPGRADE_ERROR;
				return;
			}
		}

		/* 达到最大 App 大小，强制结束 */
		if ((payload_rx_len + write_buf_len) >= APP_MAX_SIZE)
		{
			if (Upgrade_FlushPayload() == 0)
			{
				upgrade_state = UPGRADE_ERROR;
				return;
			}
			Upgrade_FinishPayload();
		}
	}
}

static void Upgrade_FinishPayload(void)
{
	uint32_t payload_crc32 = crc32_finalize(payload_crc_state);
	uint32_t app_crc32 = 0;

	if (payload_rx_len == 0)
	{
		RS485_Printf("Upgrade : no payload received\r\n");
		upgrade_state = UPGRADE_ERROR;
		return;
	}

	RS485_Printf("Upgrade : received %d bytes\r\n" , payload_rx_len);
	RS485_Printf("Upgrade : payload CRC32:0x%08X\r\n" , payload_crc32);

	RS485_Printf("Upgrade : payload done\r\n");

	app_crc32 = crc32_calc((uint8_t*)APP_START_ADDR , APP_MAX_SIZE);
	RS485_Printf("Upgrade : current app crc32:0x%08X\r\n" , app_crc32);

	Upgrade_UpdateBootParam(0 , payload_rx_len , payload_crc32 , app_crc32);
	Upgrade_SaveBootParam();
	RS485_Printf("Upgrade : write BootParam done\r\n");
	
	mcu_software_reset();
}

/**
 * 读取参数页。
 */
static void Upgrade_LoadBootParam(void)
{
	for (uint16_t i = 0; i < BOOT_PARAM_SIZE; i++)
	{
		config_buf[i] = internal_flash_read_Char(BOOT_PARAM_ADDR + i);
	}

	memcpy(&my_param_sum , config_buf , sizeof(Parameter_t));
}

/**
 * 写回参数页。
 */
static void Upgrade_SaveBootParam(void)
{
	internal_flash_erase(BOOT_PARAM_ADDR);
	memcpy(config_buf , &my_param_sum , sizeof(Parameter_t));
	internal_flash_write_str_Char(BOOT_PARAM_ADDR , config_buf , sizeof(Parameter_t));
}

/**
 * 更新 BootParam 中的升级请求字段。
 *
 * Args:
 *   appVersion: 新 App 版本号（新格式无版本字段，传 0）。
 *   payload_size: 实际接收到的固件长度。
 *   payload_crc: 实际计算出的 payload CRC32。
 *   current_app_crc: 当前 App 分区 CRC32。
 */
static void Upgrade_UpdateBootParam(uint32_t appVersion , uint32_t payload_size , uint32_t payload_crc , uint32_t current_app_crc)
{
	Upgrade_LoadBootParam();

	/* 保留旧参数，BootLoader 用它判断是否需要先备份当前 App。 */
	my_param_sum.BootParam_Reserved = my_param_sum.BootParam;

	my_param_sum.BootParam.appStartAddr = APP_START_ADDR;
	my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);	//栈顶地址
	my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);	// 应用程序入口地址在应用程序起始地址后4字节(需要手动指定中断向量表的位置)
	my_param_sum.BootParam.magicWord = APP_MAGIC_WORD;
	my_param_sum.BootParam.appVersion = appVersion;
	my_param_sum.BootParam.backupCRC32 = current_app_crc;
	RS485_Printf("my_param_sum.BootParam.backupCRC32 : 0x%08X\r\n" , my_param_sum.BootParam.backupCRC32);

	my_param_sum.BootParam.updateFlag = UPDATE_REQUEST_FLAG;
	my_param_sum.BootParam.updateStatus = UPDATE_STATUS_WAIT_BOOTLOADER;
	my_param_sum.BootParam.appSize = payload_size;
	my_param_sum.BootParam.appCRC32 = payload_crc;
}

/****************************End*****************************/






