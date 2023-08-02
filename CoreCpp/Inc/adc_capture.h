#ifndef _ADC_CAPTURE_H
#define _ADC_CAPTURE_H

#include "main.h"
#include "main_cpp.h"


void ADC_CAPTURE_Init(ADC_HandleTypeDef* hadc1, TIM_HandleTypeDef *htim1);
void ADC_CAPTURE_Init(ADC_HandleTypeDef* hadc1, TIM_HandleTypeDef *htim1,
                      ADC_HandleTypeDef* hadc2, TIM_HandleTypeDef *htim2 = nullptr);
void ADC_EXTCAPTURE_Init(ADC_HandleTypeDef* hadc3, I2C_HandleTypeDef *hi2c);

void ADC1_CAPTURE_Callback();
void ADC2_CAPTURE_Callback();
void ADC3_EXTCAPTURE_Callback();

void ADC1_CAPTURE_Capture(uint16_t* adc_value, uint32_t sample_length, uint32_t fs);
void ADC2_CAPTURE_Capture(uint16_t* adc_value, uint32_t sample_length, uint32_t fs);
void ADC3_EXTCAPTURE_Capture(uint16_t* adc_value, uint32_t sample_length, uint32_t fs);


#endif
