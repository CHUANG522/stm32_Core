#include "PT100.h"
#include "gd30ad3344_standard.h"
#include <math.h>



extern uint16_t adc_16bit;
extern float ADC_Value;
extern float temp;

typedef struct {
    float adc_sampled;   
    float temp_true;     
} PT100_CalibPoint_t;

static const PT100_CalibPoint_t calibTable[] = {
    /*  adc_sampled   temp_true */
    {1.0965f,        -49.27f }, 
    {1.1223f,        -44.49f }, 
	{1.356f,           0.0f },
	{1.483f,          20.0f }, 
    {1.5350f,         33.44f }, 
    {1.5616f,         38.61f }, 
	{1.575f,          40.0f }, 
	{1.671f,          60.0f }, 
	{1.773f,          80.0f }, 
	{1.876f,         100.0f }, 
	{2.032f,         130.45f }, 
	{2.0865f,         141.11f },  
};
#define CALIB_POINT_NUM  (sizeof(calibTable) / sizeof(calibTable[0]))

//分段线性插值 
float PT100_GetTemp_FromADC(float adc_value)
{
    int low = 0, high = CALIB_POINT_NUM - 1, mid;
    
    /* 越界保护 */
    if (adc_value <= calibTable[0].adc_sampled) {
        return calibTable[0].temp_true;
    }
    if (adc_value >= 3.3f) {
        return 9999.0f;
    }
    
    /* 二分查找 */
    while (low < high - 1) {
        mid = (low + high) >> 1;
        if (adc_value < calibTable[mid].adc_sampled) {
            high = mid;
        } else {
            low = mid;
        }
    }
    
    /* 线性插值 */
    float adc_l = calibTable[low].adc_sampled;
    float adc_h = calibTable[high].adc_sampled;
    float t_l   = calibTable[low].temp_true;
    float t_h   = calibTable[high].temp_true;
    
    float ratio = (adc_value - adc_l) / (adc_h - adc_l);
    return t_l + (t_h - t_l) * ratio;
}

float Query_CH2(void)
{

	adc_16bit = ad3344_read_data16(AD3344_CONFIG);
	ADC_Value = adc_16bit*3.3f/16384.0f;
	temp = PT100_GetTemp_FromADC(ADC_Value);
	return temp;
}



