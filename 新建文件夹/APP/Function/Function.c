/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：Function.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "Function.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/
unsigned char data[50];
// CH0 变比系数（初始为1）
float frame_ch0 = 1.0f;

// CH1 变比系数（如果需要，也可以加）
float frame_ch1 = 1.0f;
//flash
uint32_t flash_id = 0;


uint16_t g_device_id = 0x0001;


typedef struct __attribute__((packed)) Parameter_SUM
{
	BootParam_t BootParam;
	BootParam_t BootParam_Reserved;
	UpdateLog_t UpdateLog;
	UserConfig_t UserConfig;
	CalibData_t CalibData;
}Parameter_t;


uint8_t config_buf[(20 * 1024)] = { 0 };



/************************ 函数定义 ************************/

void APP_HandleUpgradeRequest(void);

/************************************************************ 
 * Function :       System_Init
 * Comment  :       用于初始化MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void System_Init(void)
{
	/* BootLoader 跳转后先屏蔽中断，清掉可能遗留的状态。 */
	__disable_irq();

	/* App 链接在 APP_START_ADDR，中断向量表也要切到这里。 */
	SCB->VTOR = APP_START_ADDR;

	/* 清除挂起中断。 */
	for (uint32_t i = 0; i < 8; i++)
	{
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

	__DSB();
	__ISB();
	
	__enable_irq();
	
	//初始化
	systick_config();     // 时钟配置
	RTC_Init();
	LED_Init();
	OLED_Init();
	spi_flash_init();
	RS485_Init();
	
	
	ADC_Init();
	
	
	__enable_irq();		  //外设准备好后再开中断
	
}
/************************************************************ 
 * Function :       Init_LED_Stat
 * Comment  :       系统初始化时用LED显示状态
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-03-10 V0.1 original
************************************************************/


/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       用户程序功能: LED1闪烁
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void UsrFunction(void)
{
	
	flash_id = spi_flash_read_id();
	delay_1ms(500);
	 DAC_Set_Value(1024);
	
//    Protocol_SendHeartbeat(g_device_id);
	while (1) 
	{
		sprintf((char*)data,"hello CIMC");			
		OLED_ShowString(0,0,data,16);
		sprintf((char*)data,"2026875390");			
		OLED_ShowString(0,16,data,16);
		OLED_Refresh();
		
		
		LED1_ON();
	
		
  


        ProtocolFrame_t rx_frame;
        
        // 轮询解析帧
        if (Protocol_ParseFrame(&rx_frame)) {
            
			 if(rx_frame.frame_type != 0x01)
    {
        continue;
    }
            // 1. 检查ID：非广播且不匹配，静默丢弃
            if (rx_frame.device_id != 0xFFFF && rx_frame.device_id != g_device_id) {
                continue;
            }
            
            // 2. 根据命令字分发
            switch (rx_frame.cmd_word) {
                case 0x0101:  // 重启
                    Protocol_SendAck(g_device_id, 0x0101, (uint8_t[]){0xFF}, 1);
                    delay_1ms(100);
                    NVIC_SystemReset();
                    break;
                    
                case 0x0104: { // 查询固件版本 2.0.1.0 -> 00020001
                    uint8_t ver[4] = {0x00, 0x02, 0x00, 0x01};
                    Protocol_SendAck(g_device_id, 0x0104, ver, 4);
                    break;
                }
                
  	

                
                case 0xFFFF:  // 广播寻找设备
                    Protocol_SendHeartbeat(g_device_id);
                    break;
				
                case 0x0501:  // 升级请求
                    Protocol_SendAck(g_device_id, 0x0101, (uint8_t[]){0xFF}, 1);
					APP_HandleUpgradeRequest();
                    break;				
case 0x0201:
{
    float ch0 = Query_CH0();

    uint32_t temp;

    uint8_t f[4];

    memcpy(&temp,&ch0,4);

    f[0] = (temp >> 24) & 0xFF;
    f[1] = (temp >> 16) & 0xFF;
    f[2] = (temp >> 8 ) & 0xFF;
    f[3] = temp & 0xFF;

    Protocol_SendAck(g_device_id,0x0201,f,4);
    break;
}
case 0x0202:
{
    float ch0 = Query_CH1();     

    uint8_t f[4];

    memcpy(f, &ch0, 4);

    Protocol_SendAck(g_device_id,0x0202,f,4);

    break;
}
//case 0x0241:
//{
//    // 1. 数据长度检查
//    if(rx_frame.data_len != 4)
//    {
//        Protocol_SendError(g_device_id);
//        break;
//    }

//    // 2. 解析大端 IEEE 754 浮点数
//    uint32_t temp = (rx_frame.data[0] << 24) |
//                    (rx_frame.data[1] << 16) |
//                    (rx_frame.data[2] << 8)  |
//                    (rx_frame.data[3]);

//    float new_ratio;
//    memcpy(&new_ratio, &temp, sizeof(float)); // ARM是小端，memcpy即可得到 float

//    // 3. 存储到全局 CH0 变比变量
//    frame_ch0 = new_ratio;

//    // 4. 回复 OK
//    uint8_t ack = 0xFF;
//    Protocol_SendAck(g_device_id, 0x0241, &ack, 1);

//    break;
//}
                default:
                    Protocol_SendError(g_device_id);
                    break;
            }
        }
        
        // ... 其他任务
    }
}


/****************************End*****************************/

void mcu_software_reset(void)
{
	/* set FAULTMASK */
	__set_FAULTMASK(1);
	NVIC_SystemReset();
}





void APP_HandleUpgradeRequest(void)
{
    uint8_t config_buf[BOOT_PARAM_SIZE] = {0};
    Parameter_t param;   /* 和 Bootloader 同结构体，确保两边定义一致 */

    /* 1. 读出当前 BootParam（保留版本号、用户配置等） */
    for (uint16_t i = 0; i < BOOT_PARAM_SIZE; i++) {
        config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
    }
    memcpy(&param, config_buf, sizeof(Parameter_t));

    /* 2. 只改升级标志，其余保留 */
    param.BootParam.updateFlag    = UPDATE_REQUEST_FLAG;          /* 请求升级 */
    param.BootParam.updateStatus  = UPDATE_STATUS_WAIT_BOOTLOADER; /* 等待 Bootloader */
    param.BootParam.appSize       = 0;                             /* 固件尚未接收 */
    /* appVersion 保持当前版本，Bootloader 会用它判断是否需要备份 */

    /* 3. 写回 Flash */
    memcpy(config_buf, &param, sizeof(Parameter_t));
    internal_flash_erase(BOOT_CONFIG_ADDR);
    internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, BOOT_PARAM_SIZE);

    /* 4. 软件复位，进入 Bootloader */
    __set_FAULTMASK(1);
    NVIC_SystemReset();
}