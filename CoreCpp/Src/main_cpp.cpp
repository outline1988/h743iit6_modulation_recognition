
//
// Created by outline on .
//

#include "main_cpp.h"
#include "adc_capture.h"
#include "sigvector.h"
#include "sigvector_fm.h"
#include "sigvector_am.h"
#include "hmi_display.h"
#include "vca821_hand.h"
#include "gpio_control.h"
#include "main_old.h"



#define SAMPLE (4096)
#define AD9834_FREQ_ERROR (22)
__attribute__((section ("._dma_buffer"))) volatile uint16_t adc_value[SAMPLE * 4];
__attribute__((section ("._dma_buffer"))) volatile float32_t float_data[SAMPLE * 4];
__attribute__((section ("._dma_buffer"))) volatile float32_t fft_mag[SAMPLE * 4];

//__attribute__((section ("._dma_buffer"))) volatile struct compx adc_value_complex[SAMPLE * 4];

uint8_t MAIN_NEW_determine_dig_psk_ana_rate(Sigvector<uint16_t> &vec, float32_t fs, float32_t &rate);


int main_cpp_analog_result() {
    RETARGET_Init(&huart1);
    ADC_CAPTURE_Init(&hadc1, &htim8);   // 带同采样PC4
    ADC_EXTCAPTURE_Init(&hadc3, &hi2c3);

    while (true) {

//        float32_t fc = MAIN_determine_fc();
//        printf("%d\n", (int)fc);
//        MAIN_determine_dds(fc);
//        float32_t fs = MAIN_determine_fs(fc);
        float32_t fs = 320e3;   // 240e3
        float32_t freq_use = 0;
        uint8_t is_am = MAIN_determine_analog_modulation(fs, freq_use);

        if (is_am == 1) {
            float32_t ma = MAIN_determine_ma(fs);
            printf("ma: %d", int(ma * 1000));
        }
        else if (is_am == 0) {
            float32_t delta_f = 0;
            float32_t mf = MAIN_determine_mf(fs, delta_f);
            printf("mf: %d    delta_f: %d", int(mf * 100), int(delta_f));
        }
        else if (is_am == 2) {
            printf("CW signal");
        }
        else {
            printf("no peaks found");
        }

        printf("\n\n");
        HAL_Delay(500);
    }
    return 0;
}

int main_cpp_ad() {
    RETARGET_Init(&huart1);
    ADC_CAPTURE_Init(&hadc1, &htim8, &hadc2);   // 带通采样PC4
    ADC_EXTCAPTURE_Init(&hadc3, &hi2c3);
    AD9834_Init();
    AD9834_Select_Wave(Sine_Wave);
    AD9834_Set_Freq(FREQ_1, 2e6);
    while (true) {
        float32_t fs = 320e3;
        float32_t rate = 0;
        ADC1_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, (uint32_t)fs);  // 联调的时候去掉
        Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
                      (float32_t *)fft_mag, SAMPLE);
        vec.fft("hann");    // 联调的时候去掉
//        vec.print();
//        vec.print();
//        vec.fft_print();

        uint8_t mod_type = MAIN_NEW_determine_dig_psk_ana_rate(vec, fs, rate);


        printf("\n\n");
        HAL_Delay(500);
    }
    return 0;
}

int main_cpp()  {
    RETARGET_Init(&huart1);
    ADC_CAPTURE_Init(&hadc1, &htim8, &hadc2);   // 带通采样PC4
    ADC_EXTCAPTURE_Init(&hadc3, &hi2c3);
    HMI_Init(&huart3);
//    AD9854_Init();

    HAL_Delay(500);
    AD9834_Init();
    AD9834_Select_Wave(Sine_Wave);
//    AD9854_SINSet(2e6, 4095);
//    SI5351_SetCLK2(2e6);
    AD9834_Set_Freq(FREQ_0, 2e6 + AD9834_FREQ_ERROR);


    Vca821_hand vca821_instance(&hdac1, DAC1_CHANNEL_1);
    vca821_instance.set_gainvalue(50);  // initial gain

    Gpio_control digital_fm_am(GPIOC, GPIO_PIN_5, GPIOA, GPIO_PIN_3);
    Gpio_control psk_fsk_ask(GPIOA, GPIO_PIN_7, GPIOG, GPIO_PIN_3);

    uint8_t mode_select = 0;
    char msg[50] = {0};
    while (true) {
//        printf("hello, world\n");
        HMI_Mode(mode_select);
        main_cpp_old(mode_select, digital_fm_am, psk_fsk_ask);
        if (mode_select == 3) {     // ANALOG

        }
        else if (mode_select == 4) {    // DIGITAL

        }
        else if (mode_select == 5) {    // ANALOG & DIGITAL
            double receive_num = 0;
            HMI_Receive_num(receive_num);
            if (std::abs(receive_num - 1) < 1e-3) {
                float32_t fs = 320e3;
                float32_t rate = 0;
                ADC1_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, (uint32_t)fs);  // 联调的时候去掉
                Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
                              (float32_t *)fft_mag, SAMPLE);
                vec.fft("hann");
                uint8_t mod_type = MAIN_NEW_determine_dig_psk_ana_rate(vec, fs, rate);
                if (mod_type == 0) {    // ASK/FSK
                    vca821_instance.set_gainvalue(50);  // max gen
                    uint8_t ask_fsk = MAIN_determine_fsk_ask((float32_t *)float_data);
                    if (ask_fsk == 0) {     // ASK
                        HMI_TXT_Transmit("DIGITAL MODULATION: ASK", 0);
                        sprintf(msg, "rate: %d", (int)std::round(rate));
                        HMI_TXT_Transmit(msg, 1);
                        HMI_TXT_Transmit(" ", 2);
                        if (digital_fm_am.get_command() != 0b00 || psk_fsk_ask.get_command() != 0b10) {
                            digital_fm_am.write(0b00);
                            psk_fsk_ask.write(0b10);
                        }
                    }   // ASK
                    else if (ask_fsk == 1) {    // FSK
//                        vec.fft_print();    HAL_Delay(400);

                        float32_t fsk_h = MAIN_measure_fsk_h(fs, rate);
                        HMI_TXT_Transmit("DIGITAL MODULATION: FSK", 0);
                        sprintf(msg, "rate: %d", (int)std::round(rate));
                        HMI_TXT_Transmit(msg, 1);
                        sprintf(msg, "h: %d", (int)std::round(fsk_h * 1000));
                        HMI_TXT_Transmit(msg, 2);
                        if (digital_fm_am.get_command() != 0b00 || psk_fsk_ask.get_command() != 0b01) {
                            digital_fm_am.write(0b00);
                            psk_fsk_ask.write(0b01);
                        }
                    }   // FSK
                }   // ASK/FSK
                else if (mod_type == 1) {   // PSK
                    vca821_instance.set_gainvalue(50);  // max gen
                    HMI_TXT_Transmit("DIGITAL MODULATION: PSK", 0);
                    sprintf(msg, "rate: %d", (int)std::round(rate));
                    HMI_TXT_Transmit(msg, 1);
                    HMI_TXT_Transmit(" ", 2);
                    if (digital_fm_am.get_command() != 0b00 || psk_fsk_ask.get_command() != 0b00) {
                        digital_fm_am.write(0b00);
                        psk_fsk_ask.write(0b00);
//                        AD9854_SINSet(2e6, 4095);
//                        SI5351_SetCLK2(2e6);

                        AD9834_Set_Freq(FREQ_1, 2e6 + AD9834_FREQ_ERROR);
                    }
                }   // PSK
                else if (mod_type == 2) {   // ANALOG
                    float32_t freq_use = 0;
                    uint8_t is_am = MAIN_determine_analog_modulation(fs, freq_use);

                    if (is_am == 1) {
                        float32_t ma = MAIN_determine_ma(fs);
                        HMI_TXT_Transmit("ANALOG MODULATION: AM", 0);
                        sprintf(msg, "MA: %d", (int)std::round(ma * 1000));
                        HMI_TXT_Transmit(msg, 1);
                        sprintf(msg, "FREQ: %d", (int)std::round(freq_use));
                        HMI_TXT_Transmit(msg, 2);
                        if (digital_fm_am.get_command() != 0b10) {
                            digital_fm_am.write(0b10);
                        }
                    }
                    else if (is_am == 0) {
                        float32_t delta_f = 0;
                        float32_t mf = MAIN_determine_mf(fs, delta_f);
                        HMI_TXT_Transmit("ANALOG MODULATION: FM", 0);
                        sprintf(msg, "MF: %d    DEV: %d", (int)std::round(mf * 1000), (int)std::round(freq_use * mf));
                        HMI_TXT_Transmit(msg, 1);

                        sprintf(msg, "FREQ: %d", (int)std::round(freq_use));
                        HMI_TXT_Transmit(msg, 2);
                        if (digital_fm_am.get_command() != 0b01) {
                            digital_fm_am.write(0b01);
                        }
                    }
                    else if (is_am == 2) {
//                        vec.fft_print();    HAL_Delay(400);
                        HMI_TXT_Transmit("CW", 0);
                        HMI_TXT_Transmit("CW", 1);
                        HMI_TXT_Transmit("CW", 2);
                    }
                    MAIN_AGC_agc(vca821_instance);  // only analog should
                }   // ANALOG
            }

        }
        HAL_Delay(100);
    }
    return 0;
}

int main_cpp_agc() {
    RETARGET_Init(&huart1);
    ADC_CAPTURE_Init(&hadc1, &htim8, &hadc2);   // 带通采样PC4
    ADC_EXTCAPTURE_Init(&hadc3, &hi2c3);
    Vca821_hand vca821_instance(&hdac1, DAC1_CHANNEL_1);
    vca821_instance.set_gainvalue(1 / 2.28 * 25);  // initial gain
//    Gpio_control digital_fm_am(GPIOI, GPIO_PIN_11, GPIOC, GPIO_PIN_5);
//    Gpio_control psk_fsk_ask(GPIOG, GPIO_PIN_3, GPIOA, GPIO_PIN_7);
    while (true) {
        MAIN_AGC_agc(vca821_instance);


//        ADC2_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, 30e3);
//        Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
//                      (float32_t *)fft_mag, SAMPLE);
//
//        vec.fft("hann");      // default rectangle win and center
//        vec.fft_print();

        printf("\n\n");
        HAL_Delay(100);
    }
    return 0;
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC3) {
        ADC3_EXTCAPTURE_Callback();
    }
    if (hadc->Instance == ADC2) {
        ADC2_CAPTURE_Callback();
    }
    if (hadc->Instance == ADC1) {
        ADC1_CAPTURE_Callback();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3 || huart->Instance == USART1) {
        HMI_Callback();
    }
}



/**
 * 通过频率间隔判断出PSK ASK/FSK 模拟
 * 但是频率除了PSK向大的方向拓展
 * 模拟向小了拓展
 * 其他都必须严格在题目的范围中
 * @param vec
 * @param fs
 * @return 0表示数字调制 1表示psk 2表示模拟调制
 */
uint8_t MAIN_NEW_determine_dig_psk_ana_rate(Sigvector<uint16_t> &vec, float32_t fs, float32_t &rate) {
    vec.select_peaks(0.1, 1.5);
    if (vec.peaks_len() == 1) {
        return 2;
    }
    uint32_t i = 0;
    for (i = 0; i < vec.peaks_len(); ++i) {
        uint32_t peak_max_val = vec.peaks(0, 0);
        uint32_t peaks_val_i = vec.peaks(i, 1);
        if (peaks_val_i == peak_max_val) {
            break;  // i is peaks' index which is max val
        }
    }
    float32_t freq_l = vec.cal_freq(vec.peaks(i, 1), fs);   // 使用能量重心校正
    float32_t freq_h = vec.cal_freq(vec.peaks(i + 1, 1), fs);
//    float32_t delta_freq = (float32_t)(vec.peaks(i + 1, 1) - vec.peaks(i, 1))
//            * fs / (float32_t)SAMPLE;
    float32_t delta_freq = freq_h - freq_l;
//    printf("\nfreq_l: %d freq_h: %d delta freq: %d\n", (int)freq_l, (int)freq_h, (int)delta_freq);
    if (delta_freq > 11e3) {
        rate = delta_freq / 2;
//        printf("PSK");
        return 1;   // PSK
    }
    else if (delta_freq > 5.5e3) {
        rate = delta_freq;
//        printf("ASK/FSK");
        return 0;   // ASK/FSK
    }
    else {
        rate = delta_freq;
//        printf("ANALOG");
        return 2;   // ANALOG
    }
}
