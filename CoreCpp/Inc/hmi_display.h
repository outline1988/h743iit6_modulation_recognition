//
// Created by outline on 2023/7/5.
//

#ifndef _HMI_DISPLAY_H
#define _HMI_DISPLAY_H

#include "main_cpp.h"

void HMI_Init(UART_HandleTypeDef *huart);

void HMI_Start();

void HMI_Mode(uint8_t &mode_select);

void HMI_Receive_num(double &num);

void HMI_Callback();

void HMI_Transmit(char *msg);

void HMI_TXT_Transmit(uint32_t uint32_num, uint32_t id);

void HMI_WAVE_Transmit(uint8_t val, uint8_t channel, uint32_t id);
void HMI_WAVE_Transmit(const uint8_t *val_array, uint32_t len, uint8_t channel, uint32_t id);


void HMI_WAVE_Transmit(const uint16_t *val_array, uint32_t len, uint16_t max, uint16_t min,
                       uint8_t channel, uint32_t id);

void HMI_LINE_Transmit(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint8_t colour = 0);

void HMI_LINE_Transmit(const float32_t *array, uint32_t len, uint32_t len_max, float32_t val_max, uint8_t colour = 0);
void HMI_LINE_Transmit(const uint16_t *array, uint32_t len, uint32_t len_max, uint32_t val_max, uint8_t colour = 0);

void HMI_Refresh(uint32_t page_id);
#endif //_HMI_DISPLAY_H
