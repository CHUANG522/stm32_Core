#include "upgrade.h"

#define UPGRADE_PACKET_MAGIC    0x5AA5C33C
#define UPGRADE_MAGIC_SIZE      4
/* 主循环 while(1) 周期约 1000ms（500ms+500ms），3 次 ≈ 3 秒空闲认为结束 */
#define UPGRADE_IDLE_MAX        3

/************************ 变量定义 ************************/

typedef struct __attribute__((packed)) Parameter_SUM
{
	BootParam_t BootParam;
	BootParam_t BootParam_Reserved;
	UpdateLog_t UpdateLog;
	UserConfig_t UserConfig;
	CalibData_t CalibData;
} Parameter_t;

static Parameter_t my_param_sum = { 0 };

static uint8_t config_buf[BOOT_PARAM_SIZE] = { 0 };
static uint8_t header_buf[UPGRADE_MAGIC_SIZE] = { 0 };
static uint8_t write_buf[UPGRADE_WRITE_CHUNK_SIZE] = { 0 };

typedef enum
{
	UPGRADE_WAIT_HEADER = 0,
	UPGRADE_RECEIVE_PAYLOAD,
	UPGRADE_ERROR
} UpgradeState_t;

typedef struct
{
	uint32_t magicWord;
} UpgradeHeader_t;

static UpgradeState_t upgrade_state = UPGRADE_WAIT_HEADER;
static UpgradeHeader_t upgrade_header = { 0 };
static uint32_t header_rx_len = 0;
static uint32_t payload_rx_len = 0;
static uint32_t write_buf_len = 0;
static uint32_t payload_crc_state = 0;
static uint32_t upgrade_idle_cnt = 0;

/************************ 函数前向声明 ************************/

static void Upgrade_ResetState(void);
static uint32_t Upgrade_ReadU32BE(const uint8_t* data);
static uint8_t Upgrade_ParseHeader(void);
static uint8_t Upgrade_CheckHeader(void);
static uint8_t Upgrade_CheckBlank(uint32_t addr, uint32_t len);
static void Upgrade_EraseDownloadAreaOnBoot(void);
static uint8_t Upgrade_WritePayload(const uint8_t* data, uint32_t len);
static uint8_t Upgrade_FlushPayload(void);
static void Upgrade_HandleByte(uint8_t byte);
static void Upgrade_FinishPayload(void);
static void Upgrade_LoadBootParam(void);
static void Upgrade_SaveBootParam(void);
static void Upgrade_UpdateBootParam(uint32_t appVersion, uint32_t payload_size, uint32_t payload_crc, uint32_t current_app_crc);

/************************ 函数实现 ************************/

void Upgrade_Init(void)
{
	bootloader_config_init(&my_param_sum.BootParam, &my_param_sum.UpdateLog, &my_param_sum.UserConfig, &my_param_sum.CalibData);
	Upgrade_ResetState();
	Usart1_RxReset();
	RS485_Printf("Upgrade : ready, magic:0x%08X\r\n", UPGRADE_PACKET_MAGIC);
}


void Upgrade_Task(void)
{
	uint8_t byte = 0;
	uint8_t received = 0;

	/* 错误自动恢复，方便重新触发升级 */
	if (upgrade_state == UPGRADE_ERROR)
	{
		RS485_Printf("Upgrade : error, auto reset\r\n");
		Upgrade_ResetState();
		return;
	}

	if (Usart1_RxOverflow() != 0)
	{
		RS485_Printf("Upgrade : uart ring overflow\r\n");
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

static void Upgrade_ResetState(void)
{
	upgrade_state = UPGRADE_WAIT_HEADER;
	memset(header_buf, 0, sizeof(header_buf));
	memset(write_buf, 0xFF, sizeof(write_buf));
	memset(&upgrade_header, 0, sizeof(upgrade_header));
	header_rx_len = 0;
	payload_rx_len = 0;
	write_buf_len = 0;
	payload_crc_state = crc32_init();
	upgrade_idle_cnt = 0;
}

static uint32_t Upgrade_ReadU32BE(const uint8_t* data)
{
	return ((uint32_t)data[0] << 24) |
	       ((uint32_t)data[1] << 16) |
	       ((uint32_t)data[2] << 8) |
	       ((uint32_t)data[3]);
}

static uint8_t Upgrade_ParseHeader(void)
{
	upgrade_header.magicWord = Upgrade_ReadU32BE(&header_buf[0]);
	return 1;
}

static uint8_t Upgrade_CheckHeader(void)
{
	if (upgrade_header.magicWord != UPGRADE_PACKET_MAGIC)
	{
		RS485_Printf("Upgrade : magic error 0x%08X\r\n", upgrade_header.magicWord);
		return 0;
	}
	return 1;
}

static uint8_t Upgrade_CheckBlank(uint32_t addr, uint32_t len)
{
	for (uint32_t i = 0; i < len; i++)
	{
		if (internal_flash_read_Char(addr + i) != 0xFF)
		{
			RS485_Printf("Upgrade : download area not blank at 0x%08X\r\n", addr + i);
			return 0;
		}
	}
	return 1;
}

static void Upgrade_EraseDownloadAreaOnBoot(void)
{
	RS485_Printf("Upgrade : erase download area on boot\r\n");
	RS485_Printf("Upgrade : erase download pages:%d\r\n", DOWNLOAD_AREA_PAGE_COUNT);
	for (uint32_t i = 0; i < DOWNLOAD_AREA_PAGE_COUNT; i++)
	{
		internal_flash_erase(DOWNLOAD_AREA_ADDR + i * FLASH_PAGE_SIZE);
	}
}

static uint8_t Upgrade_WritePayload(const uint8_t* data, uint32_t len)
{
	if (len == 0)
	{
		return 1;
	}

	if (Upgrade_CheckBlank(DOWNLOAD_AREA_ADDR + payload_rx_len, len) == 0)
	{
		return 0;
	}

	internal_flash_write_str_Char(DOWNLOAD_AREA_ADDR + payload_rx_len, (uint8_t*)data, len);
	payload_crc_state = crc32_update(payload_crc_state, data, len);
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

	if (Upgrade_WritePayload(write_buf, write_len) == 0)
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
			RS485_Printf("Upgrade : header check fail\r\n");
			return;
		}

		RS485_Printf("Upgrade : magic ok 0x%08X\r\n", upgrade_header.magicWord);
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

	RS485_Printf("Upgrade : received %d bytes\r\n", payload_rx_len);
	RS485_Printf("Upgrade : payload CRC32:0x%08X\r\n", payload_crc32);
	RS485_Printf("Upgrade : payload done\r\n");

	app_crc32 = crc32_calc((uint8_t*)APP_START_ADDR, APP_MAX_SIZE);
	RS485_Printf("Upgrade : current app crc32:0x%08X\r\n", app_crc32);

	Upgrade_UpdateBootParam(0, payload_rx_len, payload_crc32, app_crc32);
	Upgrade_SaveBootParam();
	RS485_Printf("Upgrade : write BootParam done\r\n");
	
	mcu_software_reset();
}

static void Upgrade_LoadBootParam(void)
{
	for (uint16_t i = 0; i < BOOT_PARAM_SIZE; i++)
	{
		config_buf[i] = internal_flash_read_Char(BOOT_PARAM_ADDR + i);
	}
	memcpy(&my_param_sum, config_buf, sizeof(Parameter_t));
}

static void Upgrade_SaveBootParam(void)
{
	internal_flash_erase(BOOT_PARAM_ADDR);
	memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
	internal_flash_write_str_Char(BOOT_PARAM_ADDR, config_buf, sizeof(Parameter_t));
}

static void Upgrade_UpdateBootParam(uint32_t appVersion, uint32_t payload_size, uint32_t payload_crc, uint32_t current_app_crc)
{
	Upgrade_LoadBootParam();

	my_param_sum.BootParam_Reserved = my_param_sum.BootParam;

	my_param_sum.BootParam.appStartAddr = APP_START_ADDR;
	my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 0);
	my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);
	my_param_sum.BootParam.magicWord = APP_MAGIC_WORD;
	my_param_sum.BootParam.appVersion = appVersion;
	my_param_sum.BootParam.backupCRC32 = current_app_crc;
	RS485_Printf("my_param_sum.BootParam.backupCRC32 : 0x%08X\r\n", my_param_sum.BootParam.backupCRC32);

	my_param_sum.BootParam.updateFlag = UPDATE_REQUEST_FLAG;
	my_param_sum.BootParam.updateStatus = UPDATE_STATUS_WAIT_BOOTLOADER;
	my_param_sum.BootParam.appSize = payload_size;
	my_param_sum.BootParam.appCRC32 = payload_crc;
}

/****************************End*****************************/



