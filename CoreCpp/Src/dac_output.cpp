//
// Created by outline on 2023/7/15.
//

// dac_output v1.0  2023/7/75

#include "dac_output.h"


static DAC_HandleTypeDef *__hdac = nullptr;
static uint32_t __channel_ext = 0;
static uint32_t __channel = 0;
static TIM_HandleTypeDef *__htim = nullptr;

void DAC_OUTPUT_Init(DAC_HandleTypeDef *hdac) {
    __hdac = hdac;
}

void DAC_OUTPUT_Init(DAC_HandleTypeDef *hdac, uint32_t channel, TIM_HandleTypeDef *htim) {
    __hdac = hdac;
    __channel = channel;
    __htim = htim;
}


void DAC_OUTPUT_Init(DAC_HandleTypeDef *hdac, uint32_t channel, I2C_HandleTypeDef *hi2c) {
    SI5351_Init(hi2c);
    __hdac = hdac;
    __channel_ext = channel;
}

/**
 * the external channel you can't change, which is config in CubeMX, but you should tell the code;
 * @param hdac
 * @param channel_1
 * @param htim
 * @param channel_2
 * @param hi2c
 */
void DAC_OUTPUT_Init(DAC_HandleTypeDef *hdac, uint32_t channel_1, TIM_HandleTypeDef *htim,
                                              uint32_t channel_2, I2C_HandleTypeDef *hi2c) {
    DAC_OUTPUT_Init(hdac, channel_1, htim);
    DAC_OUTPUT_Init(hdac, channel_2, hi2c);
}

/**
 * Set freq of dac wave and start dac.
 * @param dac_value dac data in one period.
 * @param sample_length length of a period.
 */
static void DAC_OUTPUT_Start(uint16_t* dac_value, uint32_t sample_length, uint32_t channel) {
    HAL_TIM_Base_Start(__htim);//打开定时器6
    HAL_DAC_Start_DMA(__hdac, channel, (uint32_t *)dac_value,
                      sample_length, DAC_ALIGN_12B_R);
}

void DAC_OUTPUT_Stop() {
    if (__htim != nullptr) {
        HAL_TIM_Base_Stop(__htim);
    }
    HAL_DAC_Stop_DMA(__hdac, __channel);
}

/*
 * 通过频率得到合适的num_samples和address_step_rate
 * psc: 16bit 分频
 * cnt: 16bit 计数值
 */
static void DAC_OUTPUT_FindFactor(double freq, uint32_t sample_length, uint32_t &psc, uint32_t &cnt) {
    const uint32_t TIM_CLK = 240e6;
    const double FAC_NUM = (double)TIM_CLK / (freq * (double)sample_length);

    static const uint32_t PSC_LIST[] = {
            2, 3, 4, 5, 6, 8, 10, 12, 15, 16, 20, 24, 25,
            30, 32, 40, 48, 50, 60, 64, 75, 80, 96, 100,
            120, 125, 128, 150, 160, 192, 200, 240, 250, 256,
            300, 320, 375, 384, 400, 480, 500, 512, 600, 625,
            640, 750, 768, 800, 960, 1000, 1024, 1200, 1250, 1280,
            1500, 1536, 1600, 1875, 1920, 2000, 2400, 2500, 2560,
            3000, 3072, 3125, 3200, 3750, 3840, 4000, 4800, 5000,
            5120, 6000, 6250, 6400, 7500, 7680, 8000, 9375, 9600,
            10000, 12000, 12500, 12800, 15000, 15360, 15625, 16000,
            18750, 19200, 20000, 24000, 25000, 25600, 30000, 31250,
            32000, 37500, 38400, 40000, 46875, 48000, 50000, 60000,
            62500, 64000
    };
    const int LEN_PSC_LIST = sizeof(PSC_LIST) / sizeof(PSC_LIST[0]);
    uint32_t psc_temp = 0;
    uint32_t cnt_temp = 0;
    double freq_temp = 0;
    double error_min = 1e16;
    for (int i = 0; i < LEN_PSC_LIST; ++i) {
        psc_temp = PSC_LIST[i];
        cnt_temp = (uint32_t)round( FAC_NUM / ((double)psc_temp) );
        if (cnt_temp > 65536) {
            continue;
        }
        if (cnt_temp == 0) {
            break;
        }
        freq_temp = (double)TIM_CLK / (double)(psc_temp * cnt_temp * sample_length);
        double error = abs(freq_temp - freq);
        if ( error < error_min ) {
            error_min = error;
            psc = psc_temp;
            cnt = cnt_temp;
        }
    }
}

void DAC_OUTPUT_SetDC(uint32_t channel, uint32_t dac_value) {
    HAL_DAC_SetValue(__hdac, channel, DAC_ALIGN_12B_R, dac_value);
    HAL_DAC_Start(__hdac, channel);
}

void DAC_OUTPUT_SettingFreq(double freq, uint32_t sample_length, TIM_HandleTypeDef *htim) {
    uint32_t psc = 0;
    uint32_t cnt = 0;
    DAC_OUTPUT_FindFactor(freq, sample_length, psc, cnt);
    __HAL_TIM_SET_COUNTER(htim, 0); // cnt reset
    __HAL_TIM_SET_PRESCALER(htim, psc - 1);
    __HAL_TIM_SET_AUTORELOAD(htim, cnt - 1);
}

void DAC_OUTPUT_SettingFreq(double freq, uint32_t sample_length) {
    SI5351_SetCLK2(freq * sample_length);
}

void DAC_OUTPUT_Output(uint16_t* dac_value, uint32_t sample_length, double freq) {
    DAC_OUTPUT_SettingFreq(freq, sample_length, __htim);
    DAC_OUTPUT_Start(dac_value, sample_length, __channel);
}

void DAC_EXTOUTPUT_Output(uint16_t* dac_value, uint32_t sample_length, double freq) {
    DAC_OUTPUT_SettingFreq(freq, sample_length);
    DAC_OUTPUT_Start(dac_value, sample_length, __channel_ext);
}

