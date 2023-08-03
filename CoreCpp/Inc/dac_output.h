//
// Created by outline on 2023/7/15.
//

#ifndef _DAC_OUTPUT_H
#define _DAC_OUTPUT_H
#include "main_cpp.h"

void DAC_OUTPUT_Init(DAC_HandleTypeDef *hdac, uint32_t channel, TIM_HandleTypeDef *htim);
void DAC_OUTPUT_Init(DAC_HandleTypeDef *hdac, uint32_t channel, I2C_HandleTypeDef *hi2c);
void DAC_OUTPUT_Init(DAC_HandleTypeDef *hdac, uint32_t channel_1, TIM_HandleTypeDef *htim,
                     uint32_t channel_2, I2C_HandleTypeDef *hi2c);

void DAC_OUTPUT_Stop();


void DAC_OUTPUT_Output(uint16_t* dac_value, uint32_t sample_length, double freq);
void DAC_EXTOUTPUT_Output(uint16_t* dac_value, uint32_t sample_length, double freq);

void DAC_OUTPUT_SettingFreq(double freq, uint32_t sample_length, TIM_HandleTypeDef *htim);
void DAC_OUTPUT_SettingFreq(double freq, uint32_t sample_length);

void DAC_OUTPUT_Init(DAC_HandleTypeDef *hdac);
void DAC_OUTPUT_SetDC(uint32_t channel, uint32_t dac_value);

#endif //_DAC_OUTPUT_H
