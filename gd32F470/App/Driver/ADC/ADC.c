#include "HeaderFiles.h"

extern float g_ch0_ratio,g_ch1_ratio;


// 初始化ADC和DAC
void ADC_Init(void)
{
    // 初始化GPIO
    rcu_periph_clock_enable(ADC_GPIO_RCU);
    rcu_periph_clock_enable(ADC_RCU);

    // PC0=CH0(电位器), PC1=CH1(DAC回读)
    gpio_mode_set(ADC_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, ADC_CH0_PIN | ADC_CH1_PIN);

    // 初始化ADC
    adc_deinit();
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_sync_mode_config( ADC_SYNC_MODE_INDEPENDENT);
    adc_special_function_config(ADCX, ADC_SCAN_MODE, DISABLE);
    adc_special_function_config(ADCX, ADC_CONTINUOUS_MODE, DISABLE); // 手动触发模式
    adc_data_alignment_config(ADCX, ADC_DATAALIGN_RIGHT);
    adc_channel_length_config(ADCX, ADC_ROUTINE_CHANNEL, 1);
    adc_enable(ADCX);
    delay_1ms(1);
    adc_calibration_enable(ADCX);
  delay_1ms(10);
    // 初始化DAC
    my_dac_init();
}

// 稳健读ADC通道
uint16_t ADC_Read_Channel(uint8_t channel)
{
    // 配置通道
    adc_routine_channel_config(ADCX, 0, channel, ADC_SAMPLETIME_112);

    // 丢弃一次残留值
    adc_software_trigger_enable(ADCX, ADC_ROUTINE_CHANNEL);
    while(!adc_flag_get(ADCX, ADC_FLAG_EOC));
    (void)adc_routine_data_read(ADCX);   // 丢弃
    adc_flag_clear(ADCX, ADC_FLAG_EOC);

    // 正式采样
    adc_software_trigger_enable(ADCX, ADC_ROUTINE_CHANNEL);
    while(!adc_flag_get(ADCX, ADC_FLAG_EOC));
    uint16_t adc_val = adc_routine_data_read(ADCX);
    adc_flag_clear(ADCX, ADC_FLAG_EOC);

    return adc_val;
}

// DAC输出
void DAC_Set_Value(uint16_t value)
{
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, value);
    dac_software_trigger_enable(DAC0, DAC_OUT0);
}

// CH0 电位器电压
float Convert_CH0(uint16_t adc_val)
{
	
    return ((float)adc_val*3.3f  / 4095.0f);
}

// CH1 DAC回读电压
float Convert_CH1(uint16_t adc_val)
{
	
    return ((float)adc_val / 4095.0f) *3.3f ; // 
}


// 查询变比CH0
float  Query_CH0(void)
{
    uint16_t adc = ADC_Read_Channel(CH0_ADC_CHANNEL);
    float percent = Convert_CH0(adc) * g_ch0_ratio;
	return percent ;
 
}

// 查询变比CH1
float Query_CH1(void)
{
    uint16_t adc = ADC_Read_Channel(CH1_ADC_CHANNEL);
    float voltage = Convert_CH1(adc) * g_ch1_ratio;
   
	return    voltage;
}




float BeBytesToFloat(const uint8_t be[4])
{
    union { float f; uint8_t b[4]; } conv;
    conv.b[0] = be[3];   /* 大端转小端 */
    conv.b[1] = be[2];
    conv.b[2] = be[1];
    conv.b[3] = be[0];
    return conv.f;
}
void FloatToBeBytes(float val, uint8_t be[4])
{
    union { float f; uint8_t b[4]; } conv;
    conv.f = val;
    be[0] = conv.b[3];   /* 小端转大端 */
    be[1] = conv.b[2];
    be[2] = conv.b[1];
    be[3] = conv.b[0];
}

