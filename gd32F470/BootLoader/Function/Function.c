#include "HeaderFiles.h"
#include "Function.h"
#include <time.h>
#include "rom.h"
#include "bootloader.h"
#include "BootConfig.h"
#include "flash_layout.h"
#include "Protocol.h"
#include "oled.h"
/************************* 宏定义 *************************/
#define TRUE 1
#define FALSE 0

typedef void (*pFunction)(void);
pFunction jump2app;

#define CONFIG_SIZE BOOT_PARAM_SIZE
#define CONFIG_APP_SIZE BOOT_TRANSFER_BUFFER_SIZE
#define APP_DOWNLOAD_ADDR DOWNLOAD_AREA_ADDR

#define Switch_App_Update_Fail_Test 0

#define CMD_PREPARE_FIRMWARE    0x0502
#define CMD_EXECUTE_UPGRADE     0x0503


#define FIRMWARE_SLICE_SIZE     256         /* 固件数据切片大小 */
#define RAW_RX_SLICE_TIMEOUT    2000        /* 单个切片超时时间（毫秒） */
#define RAW_RX_TOTAL_TIMEOUT    30000       /* 整体接收超时（毫秒），防止死等 */

/* 帧类型定义（需与 Protocol.h 保持一致） */
#define FRAME_TYPE_CMD          0x01
#define FRAME_TYPE_ACK          0x02
#define FRAME_TYPE_ERROR        0xFF

/* 命令字 */
#define CMD_PREPARE_FIRMWARE    0x0502
#define CMD_EXECUTE_UPGRADE     0x0503

/************************ 变量定义 ************************/

//OLED
unsigned char data[50];


typedef struct __attribute__((packed)) Parameter_SUM
{
	BootParam_t BootParam;
	BootParam_t BootParam_Reserved;
	UpdateLog_t UpdateLog;
	UserConfig_t UserConfig;
	CalibData_t CalibData;
} Parameter_t;

Parameter_t my_param_sum = { 0 };
uint8_t config_buf[CONFIG_APP_SIZE] = { 0 };

/************************ 函数定义 ************************/

static void Analysis_ConfigForAddr(void);
static bool Download_Transport(uint32_t DownLoad_Addr);
static void Flash_Erase_Pages(uint32_t start_addr, uint32_t page_count);
static void Flash_Copy_ByPage(uint32_t dst_addr, uint32_t src_addr, uint32_t len);
void jump_to_app(void);
bool Backup_App(void);
static void Clear_Download_Area(void);

static bool Receive_Firmware_Raw(uint32_t dst_addr, uint32_t expected_size);
static bool Verify_Firmware_MagicWord(uint32_t addr);
static void Send_PrepareFirmware_Ack(uint16_t device_id, bool is_ok);

/************************************************************
 * Function :       System_Init
************************************************************/
void System_Init(void)
{
	systick_config();
	LED_Init();
	RS485_Init();
	my_timer_init();
	OLED_Init();	
	Upgrade_Init();
}

void UsrFunction(void)
{
		LED1_OFF();
		sprintf((char*)data,"2026875390");			
		OLED_ShowString(0,0,data,16);
		sprintf((char*)data,"Bootloader");			
		OLED_ShowString(0,16,data,16);
		OLED_Refresh();	
    /* 变量提前声明，避免 goto 跳过初始化 */
    bool     Backup_Result = TRUE;
    uint8_t  prepare_cmd_received = 0;

    Analysis_ConfigForAddr();
    memcpy(&my_param_sum, config_buf, sizeof(Parameter_t));
    /* ==================== 无升级请求：静默 5 秒跳转 APP ==================== */
    if (!(my_param_sum.BootParam.updateStatus == UPDATE_STATUS_WAIT_BOOTLOADER
          && my_param_sum.BootParam.updateFlag == UPDATE_REQUEST_FLAG))
    {
        delay_1ms(5000);

        my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)APP_START_ADDR;
        my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(APP_START_ADDR + 4);

        jump_to_app();
        /* 跳转失败则兜底进入接收循环 */
        goto enter_receive_loop;
    }

    /* ==================== 有升级请求：打印信息并备份 ==================== */
    RS485_Printf("BootLoader : start compare config\r\n");

    /* 版本不一致时先备份当前 App */
    if (my_param_sum.BootParam.appVersion != my_param_sum.BootParam_Reserved.appVersion)
    {
        Backup_Result = Backup_App();
        if (Backup_Result == FALSE)
        {
            RS485_Printf("BootLoader : Backup_App failed\r\n");
        }
        else
        {
            RS485_Printf("BootLoader : Backup_App success\r\n");
        }
    }
    else
    {
        RS485_Printf("BootLoader : appVersion is same\r\n");
    }

    my_param_sum.BootParam.appStartAddr = APP_START_ADDR;
    my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(APP_START_ADDR);
    my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(APP_START_ADDR + 4);

#if Switch_App_Update_Fail_Test
    my_param_sum.BootParam.updateStatus = UPDATE_STATUS_WAIT_BOOTLOADER;
    my_param_sum.BootParam.updateFlag = UPDATE_REQUEST_FLAG;
    my_param_sum.BootParam.appStartAddr = 0x810D000;
    my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr);
    my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(my_param_sum.BootParam.appStartAddr + 4);
    RS485_Printf("bootFailCount:%d\r\n", my_param_sum.BootParam.bootFailCount);
#else
    my_param_sum.BootParam.appStartAddr = APP_START_ADDR;
    my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(APP_START_ADDR);
    my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(APP_START_ADDR + 4);
#endif

    if (my_param_sum.BootParam.bootFailCount == 0xFFFF)
    {
        my_param_sum.BootParam.bootFailCount = 0;
    }

    if (my_param_sum.BootParam.bootFailCount >= 5)
    {
        RS485_Printf("jump false , version rollback\r\n");

        for (uint16_t i = 0; i < CONFIG_SIZE; i++)
        {
            config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
        }
        memcpy(&my_param_sum, config_buf, sizeof(Parameter_t));

        my_param_sum.BootParam.bootFailCount = 0;
        my_param_sum.BootParam.appVersion = my_param_sum.BootParam.backupVersion;

        memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
        internal_flash_erase(BOOT_CONFIG_ADDR);
        internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, CONFIG_SIZE);

        my_param_sum.BootParam.appStartAddr = APP_START_ADDR;
        my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(APP_START_ADDR);
        my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(APP_START_ADDR + 4);

        Flash_Erase_Pages(APP_START_ADDR, APP_PAGE_COUNT);
        Flash_Copy_ByPage(APP_START_ADDR, BACKUP_APP_ADDR, BACKUP_APP_SIZE);
    }

    RS485_Printf("BootLoader : appStackAddr:0x%08x\r\n", my_param_sum.BootParam.appStackAddr);
    RS485_Printf("BootLoader : appEntryAddr:0x%08x\r\n", my_param_sum.BootParam.appEntryAddr);
    RS485_Printf("BootLoader : appStartAddr:0x%08x\r\n", my_param_sum.BootParam.appStartAddr);
    RS485_Printf("BootLoader : appVersion:0x%08x\r\n", my_param_sum.BootParam.appVersion);
    RS485_Printf("BootLoader : updateStatus:0x%02x\r\n", my_param_sum.BootParam.updateStatus);
    RS485_Printf("BootLoader : updateFlag:0x%02x\r\n", my_param_sum.BootParam.updateFlag);
    RS485_Printf("BootLoader : magicWord:0x%08x\r\n", my_param_sum.BootParam.magicWord);
    RS485_Printf("BootLoader : appSize:%d\r\n", my_param_sum.BootParam.appSize);
    delay_1ms(1000);

    if (my_param_sum.BootParam.magicWord != APP_MAGIC_WORD)
    {
        RS485_Printf("BootLoader : param magic is false\r\n");
		
    }
	

    /* ==================== 已收固件(appSize>0)：等 0x0503 执行升级 ==================== */
    if (my_param_sum.BootParam.appSize > 0)
    {
        RS485_Printf("BootLoader : firmware already received, waiting 0x0503...\r\n");

        if (Backup_Result == FALSE)
        {
            RS485_Printf("BootLoader : backup failed, abort update\r\n");
            for (uint16_t i = 0; i < CONFIG_SIZE; i++)
            {
                config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
            }
            memcpy(&my_param_sum, config_buf, sizeof(Parameter_t));
            my_param_sum.BootParam.appVersion = my_param_sum.BootParam_Reserved.appVersion;
            my_param_sum.BootParam.updateStatus = UPDATE_STATUS_DONE;
            my_param_sum.BootParam.updateFlag = 0x00;
            my_param_sum.BootParam.bootFailCount++;
            memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
            internal_flash_erase(BOOT_CONFIG_ADDR);
            internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, CONFIG_SIZE);
            mcu_software_reset();
        }

        /* 进入 0x0503 等待循环 */
        while (1)
        {
            LED2_ON();
            delay_1ms(500);
            LED2_OFF();
            delay_1ms(500);

            ProtocolFrame_t frame;
            if (Protocol_ParseFrame(&frame))
            {
                if (frame.frame_type == FRAME_TYPE_CMD && frame.cmd_word == CMD_EXECUTE_UPGRADE)
                {
                    /* 收到 0x0503：先回复 OK，再执行搬运 */
                    uint8_t ack_data = 0xFF;
                    Protocol_SendAck(frame.device_id, CMD_EXECUTE_UPGRADE, &ack_data, 1);
                    RS485_Printf("BootLoader : recv 0x0503, ack OK, start transport\r\n");

                    bool Download_Transport_Result = Download_Transport(APP_DOWNLOAD_ADDR);

                    for (uint16_t i = 0; i < CONFIG_SIZE; i++)
                    {
                        config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
                    }
                    memcpy(&my_param_sum, config_buf, sizeof(Parameter_t));

                    if (Download_Transport_Result == TRUE)
                    {
                        my_param_sum.BootParam.appStartAddr = APP_START_ADDR;
                        my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(APP_START_ADDR);
                        my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(APP_START_ADDR + 4);
                        my_param_sum.BootParam.updateStatus = UPDATE_STATUS_DONE;
                        my_param_sum.BootParam.updateFlag = 0x00;
                        my_param_sum.BootParam.updateCount++;
                        RS485_Printf("BootLoader : app update success\r\n");
                    }
                    else
                    {
                        RS485_Printf("BootLoader : app update fail\r\n");
                        my_param_sum.BootParam.resetCount++;
                        my_param_sum.BootParam.bootFailCount++;
                    }
                    memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
                    internal_flash_erase(BOOT_CONFIG_ADDR);
                    internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, CONFIG_SIZE);
                    if (Download_Transport_Result == TRUE)
                    {
                        Clear_Download_Area();
                    }
                    mcu_software_reset();
                }
            }
            Upgrade_Task();
        }
    }

    /* ==================== 未收固件(appSize==0)：10秒倒计时等 0x0502 ==================== */
    RS485_Printf("using command to interrupt start Application\r\n");

    prepare_cmd_received = 0;

    for (int i = 10; i > 0; i--)
    {
        RS485_Printf("wait for start Application(%ds)...\r\n", i);

        for (uint32_t ms = 0; ms < 1000; ms++)
        {
            delay_1ms(1);

            ProtocolFrame_t frame;
            if (Protocol_ParseFrame(&frame))
            {
                /* ===== 新增：处理 0x0502 准备传输固件数据包 ===== */
 if (frame.frame_type == FRAME_TYPE_CMD && frame.cmd_word == CMD_PREPARE_FIRMWARE)
{
    RS485_Printf("BootLoader : recv 0x0502, start raw firmware receive\r\n");

    /* 步骤1：接收裸固件数据 */
    /* appSize==0 时，Receive_Firmware_Raw 内部会用 DOWNLOAD_AREA_SIZE */
    bool rx_ok = Receive_Firmware_Raw(APP_DOWNLOAD_ADDR,
                                      my_param_sum.BootParam.appSize);

    /* 步骤2：校验魔术字并回复 OK 或 ERROR */
    if (rx_ok && Verify_Firmware_MagicWord(APP_DOWNLOAD_ADDR))
    {
        Send_PrepareFirmware_Ack(frame.device_id, TRUE);
        RS485_Printf("BootLoader : 0x0502 magic check OK, wait 0x0503\r\n");
        prepare_cmd_received = 1;
    }
    else
    {
        Send_PrepareFirmware_Ack(frame.device_id, FALSE);
        RS485_Printf("BootLoader : 0x0502 check fail, discard\r\n");
        Clear_Download_Area();
        prepare_cmd_received = 0;
    }
}
goto handle_prepare_result;

            }
        }
    }

    /* 10秒超时：跳转 APP */
    RS485_Printf("wait for start Application(0s)...\r\n");
    RS485_Printf("Timeout, start application\r\n");

    my_param_sum.BootParam.updateFlag = 0;
    my_param_sum.BootParam.updateStatus = UPDATE_STATUS_DONE;
    memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
    internal_flash_erase(BOOT_CONFIG_ADDR);
    internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, CONFIG_SIZE);

    my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)APP_START_ADDR;
    my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(APP_START_ADDR + 4);
    jump_to_app();
    /* 跳转失败兜底 */

handle_prepare_result:
    /* 0x0502 处理成功：进入 0x0503 等待循环 */
    if (prepare_cmd_received)
    {
        while (1)
        {
            LED2_ON();
            delay_1ms(500);
            LED2_OFF();
            delay_1ms(500);

            ProtocolFrame_t frame;
            if (Protocol_ParseFrame(&frame))
            {
                if (frame.frame_type == FRAME_TYPE_CMD && frame.cmd_word == CMD_EXECUTE_UPGRADE)
                {
                    uint8_t ack_data = 0xFF;
                    Protocol_SendAck(frame.device_id, CMD_EXECUTE_UPGRADE, &ack_data, 1);
                    RS485_Printf("BootLoader : recv 0x0503, ack OK, start transport\r\n");

                    bool Download_Transport_Result = Download_Transport(APP_DOWNLOAD_ADDR);

                    for (uint16_t i = 0; i < CONFIG_SIZE; i++)
                    {
                        config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
                    }
                    memcpy(&my_param_sum, config_buf, sizeof(Parameter_t));

                    if (Download_Transport_Result == TRUE)
                    {
                        my_param_sum.BootParam.appStartAddr = APP_START_ADDR;
                        my_param_sum.BootParam.appStackAddr = *(__IO uint32_t*)(APP_START_ADDR);
                        my_param_sum.BootParam.appEntryAddr = *(__IO uint32_t*)(APP_START_ADDR + 4);
                        my_param_sum.BootParam.updateStatus = UPDATE_STATUS_DONE;
                        my_param_sum.BootParam.updateFlag = 0x00;
                        my_param_sum.BootParam.updateCount++;
                        RS485_Printf("BootLoader : app update success\r\n");
                    }
                    else
                    {
                        RS485_Printf("BootLoader : app update fail\r\n");
                        my_param_sum.BootParam.resetCount++;
                        my_param_sum.BootParam.bootFailCount++;
                    }
                    memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
                    internal_flash_erase(BOOT_CONFIG_ADDR);
                    internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, CONFIG_SIZE);
                    if (Download_Transport_Result == TRUE)
                    {
                        Clear_Download_Area();
                    }
                    mcu_software_reset();
                }
            }
            Upgrade_Task();
        }
    }

    /* ==================== 兜底：进入固件接收循环 ==================== */
enter_receive_loop:
    Clear_Download_Area();
    Upgrade_Init();

    while (1)
    {

		
        Upgrade_Task();
    }
}

/************************************************************
 * Function :       Analysis_ConfigForAddr
************************************************************/
static void Analysis_ConfigForAddr(void)
{
	for (uint16_t i = 0; i < BOOT_PARAM_SIZE; i++)
	{
		config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
	}
}

/************************************************************
 * Function :       Flash_Erase_Pages
************************************************************/
static void Flash_Erase_Pages(uint32_t start_addr, uint32_t page_count)
{
	for (uint32_t i = 0; i < page_count; i++)
	{
		internal_flash_erase(start_addr + i * FLASH_PAGE_SIZE);
		delay_1ms(30);
	}
}

/************************************************************
 * Function :       Flash_Copy_ByPage
************************************************************/
static void Flash_Copy_ByPage(uint32_t dst_addr, uint32_t src_addr, uint32_t len)
{
	uint32_t offset = 0;

	while (offset < len)
	{
		uint32_t chunk_len = len - offset;
		if (chunk_len > FLASH_PAGE_SIZE)
		{
			chunk_len = FLASH_PAGE_SIZE;
		}

		memset(config_buf, 0xFF, FLASH_PAGE_SIZE);
		for (uint32_t i = 0; i < chunk_len; i++)
		{
			config_buf[i] = internal_flash_read_Char(src_addr + offset + i);
		}
		internal_flash_write_str_Char(dst_addr + offset, config_buf, chunk_len);

		offset += chunk_len;
	}
}

/************************************************************
 * Function :       Download_Transport
************************************************************/
static bool Download_Transport(uint32_t DownLoad_Addr)
{
	RS485_Printf("BootLoader : appSize:%d\r\n", my_param_sum.BootParam.appSize);
	if (my_param_sum.BootParam.appSize == 0)
	{
		return FALSE;
	}

	if (my_param_sum.BootParam.appSize > APP_MAX_SIZE)
	{
		RS485_Printf("BootLoader : appSize over APP_MAX_SIZE\r\n");
		return FALSE;
	}

	if (my_param_sum.BootParam.appSize > DOWNLOAD_AREA_SIZE)
	{
		RS485_Printf("BootLoader : appSize over DOWNLOAD_AREA_SIZE\r\n");
		return FALSE;
	}

	Flash_Erase_Pages(my_param_sum.BootParam.appStartAddr, APP_PAGE_COUNT);
	Flash_Copy_ByPage(my_param_sum.BootParam.appStartAddr, DownLoad_Addr, my_param_sum.BootParam.appSize);

	uint32_t app_crc32 = crc32_calc((uint8_t*)my_param_sum.BootParam.appStartAddr, my_param_sum.BootParam.appSize);

	RS485_Printf("BootLoader : appCRC32:0x%08x , app_crc32:0x%08x\r\n", my_param_sum.BootParam.appCRC32, app_crc32);
	if (app_crc32 == my_param_sum.BootParam.appCRC32)
	{
		RS485_Printf("app crc32 check pass\r\n");
		return TRUE;
	}
	else
	{
		RS485_Printf("app crc32 check fail\r\n");
		return FALSE;
	}
}

/************************************************************
 * Function :       Clear_Download_Area
************************************************************/
static void Clear_Download_Area(void)
{
	RS485_Printf("BootLoader : clear download area\r\n");
	Flash_Erase_Pages(DOWNLOAD_AREA_ADDR, DOWNLOAD_AREA_PAGE_COUNT);
	RS485_Printf("BootLoader : clear download area done\r\n");
}

/************************************************************
 * Function :       mcu_software_reset
************************************************************/
void mcu_software_reset(void)
{
	__set_FAULTMASK(1);
	NVIC_SystemReset();
}

/************************************************************
 * Function :       iap_load_app
************************************************************/
void iap_load_app(uint32_t appxaddr)
{
	if (((*(__IO uint32_t*)appxaddr) & 0x2FFE0000) == 0x20000000)
	{
		__disable_irq();

		SysTick->CTRL = 0;
		SysTick->LOAD = 0;
		SysTick->VAL = 0;

		for (uint32_t i = 0; i < 8; i++)
		{
			NVIC->ICER[i] = 0xFFFFFFFF;
			NVIC->ICPR[i] = 0xFFFFFFFF;
		}

		__DSB();
		__ISB();

		SCB->VTOR = appxaddr;

		__set_MSP(*(__IO uint32_t*)appxaddr);

		jump2app = (pFunction)(*(__IO uint32_t*)(appxaddr + 4));
		jump2app();
		
		RS485_Printf("jump to app fail\r\n");
		while (1);
	}
}

void jump_to_app(void)
{
	/* 直接从 APP 区读取向量表，不依赖 BootParam 里的旧值 */
	uint32_t app_msp   = *(__IO uint32_t*)APP_START_ADDR;
	uint32_t app_entry = *(__IO uint32_t*)(APP_START_ADDR + 4);

	RS485_Printf("BootLoader : check APP vector, MSP=0x%08x, Entry=0x%08x\r\n", app_msp, app_entry);

	/* MSP 必须在 SRAM 区，入口必须在 Flash 区 */
	if ((app_msp & 0x2FFE0000) == 0x20000000 && (app_entry & 0xFF000000) == 0x08000000)
	{
		/* 更新 BootParam 为实际值（修复 BootParam 损坏的情况） */
		my_param_sum.BootParam.appStackAddr = app_msp;
		my_param_sum.BootParam.appEntryAddr = app_entry;

		iap_load_app(APP_START_ADDR);		
	}
	else
	{

		RS485_Printf("22222, APP vector invalid\r\n");

		for (uint16_t i = 0; i < CONFIG_SIZE; i++)
		{
			config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
		}
		memcpy(&my_param_sum, config_buf, sizeof(Parameter_t));

		my_param_sum.BootParam.bootFailCount++;
		RS485_Printf("start false count : %d \r\n", my_param_sum.BootParam.bootFailCount);
		memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
		internal_flash_erase(BOOT_CONFIG_ADDR);
		internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, CONFIG_SIZE);

		mcu_software_reset();
		while (1);
	}
}

/************************************************************
 * Function :       Backup_App
************************************************************/
bool Backup_App(void)
{
	uint32_t crc32 = crc32_calc(((uint8_t*)APP_START_ADDR), APP_MAX_SIZE);
	RS485_Printf("BootLoader : backupCRC32:0x%08x , crc32:0x%08x\r\n", my_param_sum.BootParam.backupCRC32, crc32);
	if (crc32 != my_param_sum.BootParam.backupCRC32)
	{
		RS485_Printf("BootLoader : Backup_App crc32 check fail\r\n");
		return FALSE;
	}

	Flash_Erase_Pages(BACKUP_APP_ADDR, BACKUP_APP_PAGE_COUNT);
	RS485_Printf("BootLoader : Backup_App erase done\r\n");

	Flash_Copy_ByPage(BACKUP_APP_ADDR, APP_START_ADDR, BACKUP_APP_SIZE);

	uint32_t backupCRC32_1 = crc32_calc(((uint8_t*)BACKUP_APP_ADDR), BACKUP_APP_SIZE);
	if (backupCRC32_1 != my_param_sum.BootParam.backupCRC32)
	{
		RS485_Printf("ZZZZBootLoader : Backup_App crc32 check fail\r\n");
		return FALSE;
	}
	else
	{
		RS485_Printf("ZZZZBootLoader : Backup_App crc32 check pass\r\n");
	}

	for (uint16_t i = 0; i < CONFIG_SIZE; i++)
	{
		config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
	}
	memcpy(&my_param_sum, config_buf, sizeof(Parameter_t));
	RS485_Printf("BootLoader : Backup_App version:0x%08x -> 0x%08x \r\n", my_param_sum.BootParam_Reserved.appVersion, my_param_sum.BootParam.appVersion);
	my_param_sum.BootParam.backupVersion = my_param_sum.BootParam_Reserved.appVersion;
	my_param_sum.BootParam_Reserved.appVersion = my_param_sum.BootParam.appVersion;
	memcpy(config_buf, &my_param_sum, sizeof(Parameter_t));
	internal_flash_erase(BOOT_CONFIG_ADDR);
	internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, CONFIG_SIZE);
	RS485_Printf("BootLoader : Backup_App version : 0x%08x\r\n", my_param_sum.BootParam_Reserved.appVersion);
	return TRUE;
}

/************************************************************
 * Function :       Receive_Firmware_Raw
 * Comment  :       接收裸固件数据（256字节切片，不经帧封装）
 * Parameter:       dst_addr      -- 目标写入地址
 *                  expected_size -- 期望接收的总字节数
 * Return   :       TRUE  接收完成
 *                  FALSE 接收超时
 ************************************************************/
static bool Receive_Firmware_Raw(uint32_t dst_addr, uint32_t expected_size)
{
    /* 关键修复：如果 expected_size 为 0 或无效，用 Download Area 最大容量 */
    if (expected_size == 0 || expected_size > DOWNLOAD_AREA_SIZE)
    {
        expected_size = DOWNLOAD_AREA_SIZE;
        RS485_Printf("BootLoader : appSize invalid, use max size %d\r\n", expected_size);
    }

    uint32_t received = 0;
    uint8_t  slice_buf[FIRMWARE_SLICE_SIZE];
    uint32_t slice_timeout;
    uint32_t total_timeout = RAW_RX_TOTAL_TIMEOUT;
    uint8_t  last_slice_partial = 0;  /* 标记最后一帧是否为非满切片 */

    RS485_Printf("BootLoader : wait 500ms before raw receive\r\n");
    delay_1ms(500);

    /* 清空 UART 环形缓冲区 */
    Usart1_RxReset();

    RS485_Printf("BootLoader : raw receive start, max %d bytes\r\n", expected_size);

    while (received < expected_size)
    {
        uint32_t slice_size = FIRMWARE_SLICE_SIZE;
        if (received + slice_size > expected_size)
        {
            slice_size = expected_size - received;
        }

        uint32_t slice_received = 0;
        slice_timeout = RAW_RX_SLICE_TIMEOUT;

        while (slice_received < slice_size && slice_timeout > 0 && total_timeout > 0)
        {
            if (Usart1Ring_GetCount() > 0)
            {
                uint8_t ch;
                if (Usart1_ReadByte(&ch))
                {
                    slice_buf[slice_received++] = ch;
                }
            }
            else
            {
                delay_1ms(1);
                slice_timeout--;
                total_timeout--;
            }
        }

        /* 切片超时：上位机停止发数据了 */
        if (slice_received == 0 && received > 0)
        {
            /* 已经收到一些数据，但当前切片完全超时 → 传输结束 */
            RS485_Printf("BootLoader : sender stopped, total received %d bytes\r\n", received);
            break;
        }

        /* 切片部分收到（非满且超时）→ 最后一帧，也结束 */
        if (slice_received < slice_size && slice_received > 0 && slice_timeout == 0)
        {
            /* 写入这部分数据 */
            internal_flash_write_str_Char(dst_addr + received, slice_buf, slice_received);
            received += slice_received;
            RS485_Printf("BootLoader : last partial slice %d bytes, total %d\r\n", 
                         slice_received, received);
            break;
        }

        /* 切片完全没收到且是开头 → 真超时 */
        if (slice_received == 0)
        {
            RS485_Printf("BootLoader : raw timeout at start! got 0 bytes\r\n");
            return FALSE;
        }

        /* 正常写入完整切片 */
        internal_flash_write_str_Char(dst_addr + received, slice_buf, slice_received);
        received += slice_received;

        if ((received & 0xFFF) == 0 || received >= expected_size)
        {
            RS485_Printf("BootLoader : raw progress %d/%d\r\n", received, expected_size);
        }
    }

    RS485_Printf("BootLoader : raw receive done, %d bytes total\r\n", received);

    /* 关键：把实际接收到的字节数更新到全局，供后续使用 */
    /* 如果 BootParam 里的 appSize 是 0，用实际收到的字节数 */
    if (my_param_sum.BootParam.appSize == 0)
    {
        my_param_sum.BootParam.appSize = received;
    }

    return TRUE;
}

/************************************************************
 * Function :       Verify_Firmware_MagicWord
 * Comment  :       校验固件头部的魔术字
 * Parameter:       addr -- 固件所在地址
 * Return   :       TRUE  魔术字正确
 *                  FALSE 魔术字错误
 ************************************************************/
static bool Verify_Firmware_MagicWord(uint32_t addr)
{
    uint32_t magic = *(__IO uint32_t*)addr;

    RS485_Printf("BootLoader : magic read=0x%08x, expect=0x%08x\r\n",
                 magic, APP_MAGIC_WORD);

    return (magic == APP_MAGIC_WORD);
}

/************************************************************
 * Function :       Send_PrepareFirmware_Ack
 * Comment  :       发送 0x0502 的应答帧
 *                  OK  : A5B6 0001 02 0502 01 02 FF 88B4 B6A5
 *                  ERR : A5B6 0001 FF 0502 00 02 F173 B6A5
 * Parameter:       device_id -- 设备ID
 *                  is_ok     -- TRUE=OK, FALSE=ERROR
 ************************************************************/
static void Send_PrepareFirmware_Ack(uint16_t device_id, bool is_ok)
{
    if (is_ok)
    {
        /* 正确应答：帧类型 0x02，携带 1 字节数据 0xFF */
        uint8_t ack_data = 0xFF;
        Protocol_SendAck(device_id, CMD_PREPARE_FIRMWARE, &ack_data, 1);
        RS485_Printf("BootLoader : reply OK(0xFF) to 0x0502\r\n");
    }
    else
    {
        /* 错误应答：帧类型 0xFF，手动组帧 */
        uint8_t  tx_buf[16];
        uint16_t idx = 0;

        tx_buf[idx++] = 0xA5;
        tx_buf[idx++] = 0xB6;
        tx_buf[idx++] = (uint8_t)(device_id >> 8);
        tx_buf[idx++] = (uint8_t)(device_id);
        tx_buf[idx++] = FRAME_TYPE_ERROR;           /* 0xFF */
        tx_buf[idx++] = (uint8_t)(CMD_PREPARE_FIRMWARE >> 8);
        tx_buf[idx++] = (uint8_t)(CMD_PREPARE_FIRMWARE);
        tx_buf[idx++] = 0x00;                       /* DataLen = 0 */
        tx_buf[idx++] = 0x02;                       /* Seq */

        /* CRC16-Modbus 计算：DevID(2B) + FrameType(1B) + CmdWord(2B) + DataLen(1B) + Seq(1B) = 7B */
        uint16_t crc = CRC16_Modbus(&tx_buf[2], idx - 2);
        tx_buf[idx++] = (uint8_t)(crc >> 8);
        tx_buf[idx++] = (uint8_t)(crc);
        tx_buf[idx++] = 0xB6;                       /* Tail */
        tx_buf[idx++] = 0xA5;

        RS485_SendData(tx_buf, idx);
        RS485_Printf("BootLoader : reply ERROR to 0x0502\r\n");
    }
}
/****************************End*****************************/
