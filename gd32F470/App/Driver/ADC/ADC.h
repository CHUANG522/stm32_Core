#ifndef __ADC_H__
#define __ADC_H__

#include "HeaderFiles.h"

// ADC 配置
#define ADC_RCU         RCU_ADC1
#define ADC_GPIO_RCU    RCU_GPIOC
#define ADC_GPIO_PORT   GPIOC
#define ADC_CH0_PIN     GPIO_PIN_0
#define ADC_CH1_PIN     GPIO_PIN_1

#define ADCX            ADC1

// ADC通道
#define CH0_ADC_CHANNEL ADC_CHANNEL_10   // 电位器
#define CH1_ADC_CHANNEL ADC_CHANNEL_11   // DAC回读

// DAC最大电压，按实际硬件修改
#define CH1_VOLT_MAX    10.0f   // 例：实际满量程10V

// 函数声明
void ADC_Init(void);
uint16_t ADC_Read_Channel(uint8_t channel);
void DAC_Set_Value(uint16_t value);

float Convert_CH0(uint16_t adc_val);
float Convert_CH1(uint16_t adc_val);


float  Query_CH0(void);
float Query_CH1(void);

void float_to_ieee754_hex(float val, char out[9]);
float ieee754_hex_to_float(const char hex[9]);
float BeBytesToFloat(const uint8_t be[4]);
void FloatToBeBytes(float val, uint8_t be[4]);
#endif


