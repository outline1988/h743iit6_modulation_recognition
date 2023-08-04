//
// Created by outline on 2023/8/3.
//

#ifndef _MAIN_OLD_H
#define _MAIN_OLD_H
#include "main_cpp.h"
#include "adc_capture.h"
#include "sigvector.h"
#include "sigvector_fm.h"
#include "sigvector_am.h"
#include "hmi_display.h"
#include "vca821_hand.h"
#include "gpio_control.h"
#include "main_old.h"

void main_cpp_old(uint8_t &mode_select, Gpio_control &digital_fm_am, Gpio_control &psk_fsk_ask);

float32_t MAIN_determine_fc();
float32_t MAIN_determine_fs(float32_t fc);
float32_t MAIN_determine_ma(float32_t fs);
float32_t MAIN_determine_mf(float32_t fs, float32_t &delta_f);
uint8_t MAIN_determine_analog_modulation(float32_t fs, float32_t &freq_use);
void MAIN_determine_dds(float32_t fc);
uint8_t MAIN_determine_psk_rate(Sigvector<uint16_t> &vec, float32_t fs, float32_t &rate);
uint8_t MAIN_determine_fsk_ask(const float32_t *f_data);
uint8_t MAIN_determine_digital_modulation(float32_t fs, float32_t &rate);
float32_t MAIN_measure_fsk_h(float32_t fs, float32_t rate);

void MAIN_AGC_agc(Vca821_hand &vca821_instance);


#endif //_MAIN_OLD_H
