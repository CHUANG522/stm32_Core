#include "HeaderFiles.h"

// 初始化ADC和DAC
void ADC_Init(void)
{
    /* GPIO时钟 */
    rcu_periph_clock_enable(RCU_GPIOC);

    /* ADC时钟 */
    rcu_periph_clock_enable(RCU_ADC0);

    /* PC0 PC1模拟输入 */
    gpio_mode_set(GPIOC,
                  GPIO_MODE_ANALOG,
                  GPIO_PUPD_NONE,
                  GPIO_PIN_0 | GPIO_PIN_1);

    adc_deinit();

    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);

    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);

    adc_special_function_config(ADC0,
                                ADC_SCAN_MODE,
                                DISABLE);

    adc_special_function_config(ADC0,
                                ADC_CONTINUOUS_MODE,
                                DISABLE);

    adc_data_alignment_config(ADC0,
                              ADC_DATAALIGN_RIGHT);

    adc_channel_length_config(ADC0,
                              ADC_ROUTINE_CHANNEL,
                              1);

    adc_enable(ADC0);

    delay_1ms(10);

    adc_calibration_enable(ADC0);

    delay_1ms(50);

    my_dac_init();
}

// 稳健读ADC通道
uint16_t ADC_Read_Channel(uint8_t channel)
{
    adc_routine_channel_config(
        ADC0,
        0,
        channel,
        ADC_SAMPLETIME_480
    );

    adc_flag_clear(ADC0, ADC_FLAG_EOC);

    adc_software_trigger_enable(
        ADC0,
        ADC_ROUTINE_CHANNEL
    );

    while(RESET == adc_flag_get(ADC0, ADC_FLAG_EOC));

    return adc_routine_data_read(ADC0);
}
// DAC输出
void DAC_Set_Value(uint16_t value)
{
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, value);
    dac_software_trigger_enable(DAC0, DAC_OUT0);
}
extern float frame_ch0;
extern float frame_ch1;
// CH0 电位器变比换算（百分比）
float Convert_CH0(uint16_t adc_val)
{
	
    return ((float)adc_val*3.3f  / 4095.0f) *frame_ch0;
}

// CH1 DAC回读变比换算（电压）
float Convert_CH1(uint16_t adc_val)
{
	
    return ((float)adc_val / 4095.0f)*frame_ch1 ; // 
}

// 查询CH0
float  Query_CH0(void)
{
    uint16_t adc = ADC_Read_Channel(CH0_ADC_CHANNEL);
    float percent = Convert_CH0(adc);
	return percent ;
 
}

// 查询CH1
float Query_CH1(void)
{
    uint16_t adc = ADC_Read_Channel(CH1_ADC_CHANNEL);
    float voltage = Convert_CH1(adc);
   
	return    voltage;
}
