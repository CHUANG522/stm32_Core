#include "Function.h"
#include "Command.h"
#include "ADC.h"
#include "PT100.h"
#include "spi_port.h"
#include "gd30ad3344_standard.h"

uint16_t adc_16bit;
float ADC_Value;
float temp;
float pga;

extern uint16_t LED1_count;

unsigned char data[50];

uint32_t flash_id = 0;
static const uint8_t INIT_VERSION[4] = {0x02, 0x00, 0x01, 0x00};
uint8_t ver[4] = {0};

uint8_t rtc_err;

void spi_flash_write_version(uint32_t addr)
{
    spi_flash_buffer_erase(addr, 4);
    spi_flash_buffer_write((uint8_t *)INIT_VERSION, addr, 4);
}

void spi_flash_read_version(uint32_t addr, uint8_t ver[4])
{
    spi_flash_buffer_read(ver, addr, 4);
}

uint16_t version_write_done;

uint8_t device_id[2] = {0x00,0x08};
uint16_t g_device_id = 0x0008;
uint8_t s_baudrate_code = 0x13;

volatile float g_ch0_ratio = 1.0,g_ch1_ratio = 1.0;
uint8_t g_ch0_ratio_arr[4],g_ch1_ratio_arr[4];
volatile uint8_t  g_auto_report_enabled = 0;
volatile uint16_t g_report_interval_tick = 100;
volatile uint16_t g_report_tick_counter = 0;

volatile float g_ch0_threshold = 1.0f;
volatile float g_ch1_threshold = 1.0f;
volatile float g_ch2_threshold = 50.0f;
uint8_t g_ch0_threshold_arr[4],g_ch1_threshold_arr[4],g_ch2_threshold_arr[4];

uint32_t last_report_tick = 0;

typedef struct __attribute__((packed)) Parameter_SUM
{
	BootParam_t BootParam;
	BootParam_t BootParam_Reserved;
	UpdateLog_t UpdateLog;
	UserConfig_t UserConfig;
	CalibData_t CalibData;
}Parameter_t;

uint8_t config_buf[(20 * 1024)] = { 0 };

uint8_t first_flag = 0;

void AD3344_reg_Config(uint8_t InputMUX, uint8_t Channel)
{
    if(InputMUX == AD3344_DUAL_END)
    {
        switch (Channel)
        {
        case (0):
          AD3344_CONFIG |= AD3344_REG_CONFIG_MUX_DIFF_0_1;
          break;
        case (1):
          AD3344_CONFIG |= AD3344_REG_CONFIG_MUX_DIFF_0_3;
          break;
        case (2):
          AD3344_CONFIG |= AD3344_REG_CONFIG_MUX_DIFF_1_3;
          break;
        case (3):
          AD3344_CONFIG |= AD3344_REG_CONFIG_MUX_DIFF_2_3;
          break;
        }
    }else if(InputMUX == AD3344_SINGLE_END)
    {
        switch (Channel)
        {
        case (0):
          AD3344_CONFIG |= AD3344_REG_CONFIG_MUX_SINGLE_0;
          break;
        case (1):
          AD3344_CONFIG |= AD3344_REG_CONFIG_MUX_SINGLE_1;
          break;
        case (2):
          AD3344_CONFIG |= AD3344_REG_CONFIG_MUX_SINGLE_2;
          break;
        case (3):
          AD3344_CONFIG |= AD3344_REG_CONFIG_MUX_SINGLE_3;
          break;
        }
    }
    
    AD3344_CONFIG |= AD3344_REG_CONFIG_DR_1000SPS;
    AD3344_CONFIG |= AD3344_REG_CONFIG_PULL_UP_EN;
    AD3344_CONFIG |= AD3344_REG_CONFIG_NOP_VALID;
    
    AD3344_CONFIG |= AD3344_REG_CONFIG_PGA_4_096V;
    
    if((AD3344_CONFIG&AD3344_REG_CONFIG_PGA_MASK) == AD3344_REG_CONFIG_PGA_6_144V){
        pga = 6.144;
    }else if((AD3344_CONFIG&AD3344_REG_CONFIG_PGA_MASK) == AD3344_REG_CONFIG_PGA_4_096V){
        pga = 4.096;
    }else if((AD3344_CONFIG&AD3344_REG_CONFIG_PGA_MASK) == AD3344_REG_CONFIG_PGA_2_048V){
        pga = 2.048;
    }else if((AD3344_CONFIG&AD3344_REG_CONFIG_PGA_MASK) == AD3344_REG_CONFIG_PGA_1_024V){
        pga = 1.024;
    }else if((AD3344_CONFIG&AD3344_REG_CONFIG_PGA_MASK) == AD3344_REG_CONFIG_PGA_0_512V){
        pga = 0.512;
    }else if((AD3344_CONFIG&AD3344_REG_CONFIG_PGA_MASK) == AD3344_REG_CONFIG_PGA_0_256V){
        pga = 0.256;
    }else{
        pga = 0.064;
    }
    
#ifdef CONTINUOUS_CONVERSION
        AD3344_CONFIG |= AD3344_REG_CONFIG_MODE_CONTIN;
#else
        AD3344_CONFIG |= AD3344_REG_CONFIG_MODE_SINGLE;
        AD3344_CONFIG |= AD3344_REG_CONFIG_OS_SINGLE;
#endif
}

void APP_ProcessFrame(ProtocolFrame_t *frame);
void IWDG_Init(void)
{
    rcu_osci_on(RCU_IRC32K);
    while (ERROR == rcu_osci_stab_wait(RCU_IRC32K));

    fwdgt_config(1875, FWDGT_PSC_DIV256);
    fwdgt_enable();
}

void System_Init(void)
{
	__disable_irq();

	SCB->VTOR = APP_START_ADDR;

	for (uint32_t i = 0; i < 8; i++)
	{
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

	__DSB();
	__ISB();
	
	__enable_irq();
	
	systick_config();
	LED_Init();
	rtc_err = RTC_Init();

	
	OLED_Init();
	spi_flash_init();
	ADC_Init();
	
	RS485_Init();
	IWDG_Init();
	my_timer_init();
	
	__enable_irq();		

	spi_flash_buffer_read(&first_flag, 0x001B, 1);
	delay_1ms(1);

if (first_flag != 0xA5)
{
    uint8_t device_id[2]        = {0x00, 0x08};
    uint8_t baud_code           = 0x13;
	
	uint8_t dac_be[2];
    dac_be[0] = 0x00;
    dac_be[1] = 0xFF;
	
	FloatToBeBytes(g_ch0_ratio,g_ch0_ratio_arr);
	FloatToBeBytes(g_ch1_ratio,g_ch1_ratio_arr);
	FloatToBeBytes(g_ch0_threshold,g_ch0_threshold_arr);
	FloatToBeBytes(g_ch1_threshold,g_ch1_threshold_arr);	
	FloatToBeBytes(g_ch2_threshold,g_ch2_threshold_arr);
	uint8_t first_flag = 0xA5;

    spi_flash_sector_erase(0x0000);
    delay_1ms(50);

    spi_flash_buffer_write(device_id,            0x0004, 2);  delay_1ms(5);
    spi_flash_buffer_write(&baud_code,           0x0006, 1);  delay_1ms(5);
    spi_flash_buffer_write(g_ch0_ratio_arr,      0x0007, 4);  delay_1ms(5);
    spi_flash_buffer_write(g_ch1_ratio_arr,      0x000B, 4);  delay_1ms(5);
    spi_flash_buffer_write(g_ch0_threshold_arr,  0x000F, 4);  delay_1ms(5);
    spi_flash_buffer_write(g_ch1_threshold_arr,  0x0013, 4);  delay_1ms(5);
    spi_flash_buffer_write(g_ch2_threshold_arr,  0x0017, 4);  delay_1ms(5);
	spi_flash_buffer_write(&first_flag,          0x001B, 1);  delay_1ms(5);
	spi_flash_buffer_write(dac_be,              0x001C, 2);  delay_1ms(5);
	DAC_Set_Value(255);
}
	uint8_t dac_value[2];
	spi_flash_buffer_read(dac_value, 0x001C, 2);
	DAC_Set_Value( (uint16_t)dac_value[0] << 8 | dac_value[1] );
	


	uint8_t id[2];
	spi_flash_buffer_read(id, 0x0004, 2);
	g_device_id = (id[0]<<8 | id[1]);
	
	
    uint8_t baud_code = 0x13;
    spi_flash_buffer_read(&baud_code, 0x0006, 1);
    if (baud_code != 0x11 && baud_code != 0x12 && 
        baud_code != 0x13 && baud_code != 0x14) {
        baud_code = 0x13;
    }
    s_baudrate_code = baud_code;
    
    if (baud_code != 0x13) {
        RS485_ReInit(Baudrate_CodeToValue(baud_code));		
    }

	uint8_t g_ch0_ratio_arr[4];
	spi_flash_buffer_read(g_ch0_ratio_arr, 0x0007, 4);
	delay_1ms(1);
	g_ch0_ratio = BeBytesToFloat(g_ch0_ratio_arr);
	
	uint8_t g_ch1_ratio_arr[4];
	spi_flash_buffer_read(g_ch1_ratio_arr, 0x000B, 4);
	delay_1ms(1);	
	g_ch1_ratio = BeBytesToFloat(g_ch1_ratio_arr);
	
	uint8_t g_ch0_threshold_arr[4];
	spi_flash_buffer_read(g_ch0_threshold_arr, 0x000F, 4);
	delay_1ms(1);
	g_ch0_threshold = BeBytesToFloat(g_ch0_threshold_arr);
	
	uint8_t g_ch1_threshold_arr[4];
	spi_flash_buffer_read(g_ch1_threshold_arr, 0x0013, 4);
	delay_1ms(1);
	g_ch1_threshold = BeBytesToFloat(g_ch1_threshold_arr);

	uint8_t g_ch2_threshold_arr[4];
	spi_flash_buffer_read(g_ch2_threshold_arr, 0x0017, 4);
	delay_1ms(1);
	g_ch2_threshold = BeBytesToFloat(g_ch2_threshold_arr);
	
}

void UsrFunction(void)
{
if (rtc_err != 0) {
    RS485_Printf("RTC Init failed: %d\r\n", rtc_err);
}
	flash_id = spi_flash_read_id();
	if(flash_id == 0xC84013 && !version_write_done)
	{
		spi_flash_write_version(0x0000);
		delay_1ms(1);		
		version_write_done = 1;
	}
	

    ad3344_spi_init();	 
    ad3344_process(); 
    ad3344_ExtRef();   
    AD3344_reg_Config(AD3344_SINGLE_END, 0);
    ad3344_init(AD3344_CONFIG);	
	ad3344_Exti_disable();

    Protocol_SendHeartbeat(g_device_id);
	Usart1_RxReset();
    while (1) 
	{			
		delay_1ms(100);
				
		sprintf((char*)data,"2026875390");			
		OLED_ShowString(0,0,data,16);
		if(g_auto_report_enabled)
		{
			LED2_ON();
			sprintf((char*)data,"AutoSample");	
		}
		else
		{
			LED2_OFF();
			sprintf((char*)data,"IDLE      ");	
		}
		
		OLED_ShowString(0,16,data,16);
		delay_1ms(10);
		OLED_Refresh();
		
		if(LED1_count>=100)
		{
			LED1_count = 0;
			LED1_TOGGLE();
		}
				
		ProtocolFrame_t frame;
        uint8_t parsed = 0;

        while (Protocol_ParseFrame(&frame))
        {
            Cmd_ProcessFrame(&frame);
            parsed = 1;
			Usart1_RxReset();
            fwdgt_counter_reload();
        }
        fwdgt_counter_reload();

        if (!parsed) {
            delay_1ms(10);
        }
		
    }
      }

void mcu_software_reset(void)
{	
	__set_FAULTMASK(1);
	NVIC_SystemReset();
}

void APP_HandleUpgradeRequest(void)
{
    uint8_t config_buf[BOOT_PARAM_SIZE] = {0};
    Parameter_t param;

    for (uint16_t i = 0; i < BOOT_PARAM_SIZE; i++) {
        config_buf[i] = internal_flash_read_Char(BOOT_CONFIG_ADDR + i);
    }
    memcpy(&param, config_buf, sizeof(Parameter_t));

    param.BootParam.updateFlag    = UPDATE_REQUEST_FLAG;
    param.BootParam.updateStatus  = UPDATE_STATUS_WAIT_BOOTLOADER;
    param.BootParam.appSize       = 0;

    memcpy(config_buf, &param, sizeof(Parameter_t));
    internal_flash_erase(BOOT_CONFIG_ADDR);
    internal_flash_write_str_Char(BOOT_CONFIG_ADDR, config_buf, BOOT_PARAM_SIZE);

    __set_FAULTMASK(1);
    NVIC_SystemReset();
}