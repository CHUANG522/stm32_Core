/************************************************************
 * ©2026CIMC Copyright©
 * 文件名: Function.c (改进版 v2.1)
 * 说明: 用户业务逻辑 - 正确的命令处理循环
 * 作者: Lingyu Meng / Copilot
 * 平台: 2025CIMC IHD-V04
 * 版本: V2.1 2026/06/06 - 修复LED兼容性问题
************************************************************/

#include "Function.h"
#include "Protocol.h"

/*======================== 全局变量 ========================*/
unsigned char data[50];
float frame_ch0 = 1.0f;  /* CH0 变比系数，初始为1倍 */
float frame_ch1 = 1.0f;  /* CH1 变比系数 */
uint32_t flash_id = 0;
uint16_t g_device_id = 0x0001;

typedef struct __attribute__((packed)) Parameter_SUM {
    BootParam_t BootParam;
    BootParam_t BootParam_Reserved;
    UpdateLog_t UpdateLog;
    UserConfig_t UserConfig;
    CalibData_t CalibData;
} Parameter_t;

uint8_t config_buf[(20 * 1024)] = { 0 };

/*======================== LED控制宏定义 ========================*/
/* 根据你的项目自行调整这些宏 */
#ifndef LED1_ON
    #define LED1_ON()   do { } while(0)
#endif

#ifndef LED1_OFF
    #define LED1_OFF()  do { } while(0)
#endif

#ifndef LED1_TOG
    #define LED1_TOG()  do { } while(0)
#endif

/*======================== 前向声明 ========================*/
void APP_HandleUpgradeRequest(void);

/*======================== 系统初始化 ========================*/
void System_Init(void)
{
    /* 禁用中断，设置中断向量表 */
    __disable_irq();
    SCB->VTOR = APP_START_ADDR;
    
    /* 清除所有待处理中断 */
    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    __DSB();
    __ISB();
    __enable_irq();
    
    /* 初始化各模块 */
    systick_config();
    RTC_Init();
    LED_Init();
    OLED_Init();
    spi_flash_init();
    RS485_Init();
    ADC_Init();
    Protocol_Init();  /* 初始化协议层 */
    
    __enable_irq();
}

/*======================== IEEE 754 浮点数处理 ========================*/
/* 将IEEE 754单精度浮点数转为4字节大端序 */
static void Protocol_FloatToBytes(float value, uint8_t *bytes)
{
    uint32_t temp;
    memcpy(&temp, &value, 4);
    bytes[0] = (uint8_t)((temp >> 24) & 0xFF);
    bytes[1] = (uint8_t)((temp >> 16) & 0xFF);
    bytes[2] = (uint8_t)((temp >> 8) & 0xFF);
    bytes[3] = (uint8_t)(temp & 0xFF);
}

/* 从4字节大端序转为IEEE 754单精度浮点数 */
static float Protocol_BytesToFloat(const uint8_t *bytes)
{
    uint32_t temp = ((uint32_t)bytes[0] << 24) |
                    ((uint32_t)bytes[1] << 16) |
                    ((uint32_t)bytes[2] << 8) |
                    ((uint32_t)bytes[3]);
    float value;
    memcpy(&value, &temp, 4);
    return value;
}

/*======================== 命令处理函数 ========================*/

/* 处理查询CH0数据命令 */
static void Handle_QueryCH0(void)
{
    float ch0 = Query_CH0() * frame_ch0;  /* 应用变比 */
    uint8_t response[4];
    Protocol_FloatToBytes(ch0, response);
    Protocol_SendAck(g_device_id, CMD_CH0_DATA, response, 4);
}

/* 处理查询CH1数据命令 */
static void Handle_QueryCH1(void)
{
    float ch1 = Query_CH1() * frame_ch1;
    uint8_t response[4];
    Protocol_FloatToBytes(ch1, response);
    Protocol_SendAck(g_device_id, CMD_CH1_DATA, response, 4);
}

/* 处理设置CH0变比命令 */
static void Handle_SetCH0Ratio(const ProtocolFrame_t *frame)
{
    if (frame->data_len != 4) {
        Protocol_SendError(g_device_id, CMD_SET_CH0_RATIO);
        return;
    }
    
    frame_ch0 = Protocol_BytesToFloat(frame->data);
    uint8_t ack = 0xFF;
    Protocol_SendAck(g_device_id, CMD_SET_CH0_RATIO, &ack, 1);
}

/* 处理设置CH1变比命令 */
static void Handle_SetCH1Ratio(const ProtocolFrame_t *frame)
{
    if (frame->data_len != 4) {
        Protocol_SendError(g_device_id, CMD_SET_CH1_RATIO);
        return;
    }
    
    frame_ch1 = Protocol_BytesToFloat(frame->data);
    uint8_t ack = 0xFF;
    Protocol_SendAck(g_device_id, CMD_SET_CH1_RATIO, &ack, 1);
}

/* 处理查询固件版本命令 */
static void Handle_QueryFWVersion(void)
{
    uint8_t version[4] = {0x02, 0x00, 0x01, 0x00};  /* 2.0.1.0 */
    Protocol_SendAck(g_device_id, CMD_FW_VERSION, version, 4);
}

/* 处理设置DAC命令 */
static void Handle_SetDAC(const ProtocolFrame_t *frame)
{
    if (frame->data_len != 2) {
        Protocol_SendError(g_device_id, CMD_SET_DAC);
        return;
    }
    
    uint16_t dac_value = ((uint16_t)frame->data[0] << 8) | frame->data[1];
    if (dac_value > 4095) {
        Protocol_SendError(g_device_id, CMD_SET_DAC);
        return;
    }
    
    DAC_Set_Value(dac_value);
    uint8_t ack = 0xFF;
    Protocol_SendAck(g_device_id, CMD_SET_DAC, &ack, 1);
}

/* 处理设备重启命令 */
static void Handle_Reboot(void)
{
    uint8_t ack = 0xFF;
    Protocol_SendAck(g_device_id, CMD_REBOOT, &ack, 1);
    delay_1ms(100);
    NVIC_SystemReset();
}

/* 处理升级请求命令 */
static void Handle_UpgradeRequest(void)
{
    uint8_t ack = 0xFF;
    Protocol_SendAck(g_device_id, CMD_UPGRADE_REQUEST, &ack, 1);
    delay_1ms(100);
    APP_HandleUpgradeRequest();
}

/*======================== 主业务循环 ========================*/

void UsrFunction(void)
{
    flash_id = spi_flash_read_id();
    delay_1ms(500);
    DAC_Set_Value(1024);
    
    /* 发送初始心跳 */
    Protocol_SendHeartbeat(g_device_id);
    
    /* LED闪烁计数器 */
    uint32_t led_blink_count = 0;
    uint8_t led_state = 0;
    
    while (1) {
        /* 更新OLED显示 */
        sprintf((char*)data, "ID:%04X", g_device_id);
        OLED_ShowString(0, 0, data, 16);
        sprintf((char*)data, "IDLE");
        OLED_ShowString(0, 16, data, 16);
        OLED_Refresh();
        
        /* 系统状态LED指示 (1秒闪烁，约2ms循环一次) */
        led_blink_count++;
        if (led_blink_count > 500) {  /* 500 * 2ms = 1秒 */
            led_state = (led_state == 0) ? 1 : 0;
            if (led_state) {
                LED1_ON();
            } else {
                LED1_OFF();
            }
            led_blink_count = 0;
        }
        
        /*==================== 协议解析循环 ====================*/
        
        ProtocolFrame_t rx_frame;
        
        /* 尝试解析接收到的帧 */
        if (Protocol_ParseFrame(&rx_frame)) {
            
            /* 验证帧类型 - 应该是命令帧 */
            if (rx_frame.frame_type != FRAME_TYPE_CMD) {
                continue;
            }
            
            /* 验证设备ID */
            if (rx_frame.device_id != BROADCAST_ADDR && 
                rx_frame.device_id != g_device_id) {
                continue;  /* 不是发给我们的帧，忽略 */
            }
            
            /* 根据命令字处理 */
            switch (rx_frame.cmd_word) {
                
                /* 系统管理类 */
                case CMD_REBOOT:
                    Handle_Reboot();
                    break;
                    
                case CMD_FW_VERSION:
                    Handle_QueryFWVersion();
                    break;
                    
                case CMD_UPGRADE_REQUEST:
                    Handle_UpgradeRequest();
                    break;
                
                /* 数据查询类 */
                case CMD_CH0_DATA:
                    Handle_QueryCH0();
                    break;
                    
                case CMD_CH1_DATA:
                    Handle_QueryCH1();
                    break;
                
                /* 参数配置类 */
                case CMD_SET_CH0_RATIO:
                    Handle_SetCH0Ratio(&rx_frame);
                    break;
                    
                case CMD_SET_CH1_RATIO:
                    Handle_SetCH1Ratio(&rx_frame);
                    break;
                
                /* 控制类 */
                case CMD_SET_DAC:
                    Handle_SetDAC(&rx_frame);
                    break;
                
                /* 广播寻址 */
                case CMD_BROADCAST_FIND:
                    if (rx_frame.device_id == BROADCAST_ADDR) {
                        Protocol_SendHeartbeat(g_device_id);
                    }
                    break;
                
                /* 其他未知命令返回错误 */
                default:
                    Protocol_SendError(g_device_id, rx_frame.cmd_word);
                    break;
            }
        }
        
        /* 短暂延迟，避免死循环占满CPU */
        delay_1ms(2);
    }
}

/*======================== 升级处理 ========================*/

void APP_HandleUpgradeRequest(void)
{
    uint8_t config_buf_local[BOOT_PARAM_SIZE] = {0};
    Parameter_t param;

    /* 读取当前BootParam配置 */
    for (uint16_t i = 0; i < BOOT_PARAM_SIZE; i++) {
        config_buf_local[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
    }
    memcpy(&param, config_buf_local, sizeof(Parameter_t));

    /* 设置升级标志 */
    param.BootParam.updateFlag    = UPDATE_REQUEST_FLAG;
    param.BootParam.updateStatus  = UPDATE_STATUS_WAIT_BOOTLOADER;
    param.BootParam.appSize       = 0;

    /* 写入Flash */
    memcpy(config_buf_local, &param, sizeof(Parameter_t));
    internal_flash_erase(BOOT_CONFIG_ADDR);
    internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf_local, BOOT_PARAM_SIZE);

    /* 软件复位，进入Bootloader */
    __set_FAULTMASK(1);
    NVIC_SystemReset();
}

/****************************End*****************************/
