
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
#define AD9834_3_FREQ_ERROR (22)
double AD9834_2_FREQ_ERROR = (26.1);       // 22 + 48
#define SI5351_FREQ_ERROR (-100 + 5 - 0)
__attribute__((section ("._dma_buffer"))) volatile uint16_t adc_value[SAMPLE * 4];
__attribute__((section ("._dma_buffer"))) volatile float32_t float_data[SAMPLE * 4];
__attribute__((section ("._dma_buffer"))) volatile float32_t fft_mag[SAMPLE * 4];



//__attribute__((section ("._dma_buffer"))) volatile struct compx adc_value_complex[SAMPLE * 4];

uint8_t MAIN_NEW_determine_dig_psk_ana_rate(Sigvector<uint16_t> &vec, float32_t fs, float32_t &rate);
uint8_t MAIN_NEW_determine_psk_fsk_ask(const float32_t *f_data);
void MAIN_NEW_determine_rate(Sigvector<uint16_t> &vec, float32_t &rate, float32_t fs);
uint8_t MAIN_NEW_determine_psk_fsk(Sigvector<uint16_t> &vec);
uint8_t MAIN_NEW_determine_pskfsk_ask(const float32_t *f_data);

void MAIN_NEW_ANALOG_MODE();
void MAIN_NEW_DIGITAL_MODE();
void MAIN_NEW_ANA_DIG_MODE();
void MAIN_NEW_SPECTRUM();


int main_cpp__() {
    RETARGET_Init(&huart1);
    HMI_Init(&huart1);
    ADC_CAPTURE_Init(&hadc1, &htim8, &hadc2);   // 带通采样PC4
    ADC_EXTCAPTURE_Init(&hadc3, &hi2c3);
    AD9834_Init();
    AD9834_Select_Wave(Sine_Wave);
    AD9834_Set_Freq(FREQ_0, 2e6 + AD9834_2_FREQ_ERROR);
    float32_t fs = 5e6;
    while (true) {
//        ADC3_EXTCAPTURE_Capture((uint16_t *)adc_value, SAMPLE, fs);
//        Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
//                      (float32_t *)fft_mag, SAMPLE);
//        vec.fft("hann");
//        vec.fft_print();
        float32_t fc = MAIN_determine_fc();
        printf("fc: %d", (int)fc);
        printf("\n\n");
        HAL_Delay(500);
    }
    return 0;
}

Gpio_control digital_fm_am(GPIOC, GPIO_PIN_5, GPIOA, GPIO_PIN_3);
Gpio_control psk_fsk_ask(GPIOA, GPIO_PIN_7, GPIOG, GPIO_PIN_3);
Gpio_control onekhz_twokhz(GPIOA, GPIO_PIN_2, GPIOI, GPIO_PIN_8);
Vca821_hand vca821_instance(&hdac1, DAC1_CHANNEL_1);
float32_t fm_freq_use_record = 0;

int main_cpp()  {
    RETARGET_Init(&huart1);
    ADC_CAPTURE_Init(&hadc1, &htim8, &hadc2);   // 带通采样PC4
    ADC_EXTCAPTURE_Init(&hadc3, &hi2c3);
    HMI_Init(&huart3);
//    AD9854_Init();

    HAL_Delay(500);
    AD9834_Init();
    AD9834_Select_Wave(Sine_Wave);
    AD9834_Set_Freq(FREQ_0, 2e6 + AD9834_2_FREQ_ERROR);
//    SI5351_SetCLK2(2e6 + SI5351_FREQ_ERROR);

    vca821_instance.set_gainvalue(50);  // initial gain
    onekhz_twokhz.write(0b01);
    uint8_t mode_select = 0;
    char msg[50] = {0};

    double ad9854_current_error = 26.1;
    while (true) {
//        printf("hello, world\n");
        HMI_Mode(mode_select);
        main_cpp_old(mode_select, digital_fm_am, psk_fsk_ask);  // 旧模式

        if (mode_select == 3) {     // ANALOG
            MAIN_NEW_ANALOG_MODE();
        }   // ANALOG SIN
        else if (mode_select == 4) {    // DIGITAL
            MAIN_NEW_DIGITAL_MODE();
        }   // DIGITAL SIN
        else if (mode_select == 5) {    // ANALOG & DIGITAL
            MAIN_NEW_ANA_DIG_MODE();
        }   // ANALOG & DIGITAL
        else if (mode_select == 6) {
            double receive_num = 200;
            HMI_Receive_num(receive_num);
            AD9834_2_FREQ_ERROR = 24.1 + receive_num / 100;          // 串口屏显示10表示26.1    串口屏每加1 代表实际频率0.1
            if (std::abs(AD9834_2_FREQ_ERROR - ad9854_current_error) > 1e-3) {
                ad9854_current_error = AD9834_2_FREQ_ERROR;
                AD9834_Set_Freq(FREQ_0, 2e6 + AD9834_2_FREQ_ERROR);
            }
        }   // CALIBRATION
        else if (mode_select == 7) {
            MAIN_NEW_SPECTRUM();
        }
        else if (mode_select == 8) {
            float32_t fc = MAIN_determine_fc();
            double receive_num = 0;
            HMI_Receive_num(receive_num);
            if (std::abs(receive_num - 1) < 1e-3) {
                HMI_TXT_INT_Transmit(fc, 1);
            }
        }
        HAL_Delay(50);
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

/**
 * 通过时域方法分别出ask和其他波形
 * @param f_data
 * @return
 */
uint8_t MAIN_NEW_determine_pskfsk_ask(const float32_t *f_data) {
    float32_t max_time_val = 0;
    uint32_t max_time_index = 0;
    arm_max_f32(f_data, SAMPLE, &max_time_val, &max_time_index);
    float32_t accumulate_result = 0;
    for (uint32_t i = 0; i < SAMPLE; ++i) {
        accumulate_result += std::abs(f_data[i]) / max_time_val;
    }
    if (accumulate_result > 1000) {
        return 0;   // FSK  PSK
    }
    else {
        return 1;   // ASK
    }
}

/**
 * 算出rate
 * @param vec
 * @param fs
 * @return
 */
void MAIN_NEW_determine_rate(Sigvector<uint16_t> &vec, float32_t &rate, float32_t fs) {
    vec.select_peaks(0.2, 1.5);
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
    rate = delta_freq;
    return;
}

/**
 * 0返回psk 1返回fsk，通过固定阈值谱数量
 * @param vec
 * @return
 */
uint8_t MAIN_NEW_determine_psk_fsk(Sigvector<uint16_t> &vec) {
    vec.select_peaks(0.6, 1.5);
    uint32_t peaks_len = vec.peaks_len();
//    printf("%d", (int)peaks_len);
    if (peaks_len < 3) {
        return 0;
    }
    else {
        return 1;
    }
}

void MAIN_NEW_ANALOG_MODE() {
    float32_t fs = 320e3;
    double receive_num = 0;
    HMI_Receive_num(receive_num);
    if (std::abs(receive_num - 1) < 1e-3) {
        float32_t freq_use = 0;
        uint8_t is_am_array[3] = {3, 3, 3};
        for (uint8_t i = 0; i < 3; ++i) {
            is_am_array[i] = MAIN_determine_analog_modulation(fs, freq_use);
        }
        uint8_t is_am = (is_am_array[0] == is_am_array[1]) && (is_am_array[1] == is_am_array[2]) ? is_am_array[0] : 3;
        char msg[50] = {0};
        if (is_am == 1) {       // AM
            float32_t ma = MAIN_determine_ma(fs);
            HMI_AM_Transmit(freq_use, ma);

            if (digital_fm_am.get_command() != 0b10) {
                onekhz_twokhz.write(0b01);
                digital_fm_am.write(0b10);
            }
        }
        else if (is_am == 0) {      // FM
            float32_t delta_f = 0;
            float32_t mf = MAIN_determine_mf(fs, delta_f, 0.1, 1.5, 0.6);
            HMI_FM_Transmit(freq_use, mf);

            if (freq_use != fm_freq_use_record) {   // GPIO 1k 2k 窄带滤波器
                fm_freq_use_record = freq_use;
                if ( std::abs(freq_use - 1e3) < 500 ) {
                    onekhz_twokhz.write(0b00);
                }
                else if (std::abs(freq_use - 2e3) < 500 || std::abs(freq_use - 3e3) < 500) {
                    onekhz_twokhz.write(0b11);
                }
                else if (std::abs(freq_use - 4e3) < 500 || std::abs(freq_use - 5e3) < 500) {
                    onekhz_twokhz.write(0b10);
                }
                else {
                    onekhz_twokhz.write(0b01);
                }
            }
            if (digital_fm_am.get_command() != 0b01) {
                // 窄带滤波器
                AD9834_Set_Freq(FREQ_1, 3e6 + AD9834_3_FREQ_ERROR);
                digital_fm_am.write(0b01);
            }
        }
        else if (is_am == 2) {      // CW
            HMI_CW_Transmit();
        }
        MAIN_AGC_agc(vca821_instance);  // only analog should
    }
}

void MAIN_NEW_DIGITAL_MODE() {
    float32_t fs = 320e3;
    float32_t rate = 0;
    double receive_num = 0;
    HMI_Receive_num(receive_num);
    if (std::abs(receive_num - 1) < 1e-3) {
        ADC1_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, (uint32_t)fs);  // 联调的时候去掉
        Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
                      (float32_t *)fft_mag, SAMPLE);
        vec.fft("hann");    // 联调的时候去掉

        uint8_t pskfsk_ask = MAIN_NEW_determine_pskfsk_ask((float32_t *)float_data);
        MAIN_NEW_determine_rate(vec, rate, fs);
        if (pskfsk_ask == 0) {
            uint8_t psk_fsk = MAIN_NEW_determine_psk_fsk(vec);
            if (psk_fsk == 0) {     // PSK
                rate /= 2;
                vca821_instance.set_gainvalue(50);  // max gen

                HMI_PSK_Transmit(rate);

                if (digital_fm_am.get_command() != 0b00 || psk_fsk_ask.get_command() != 0b00) {
                    onekhz_twokhz.write(0b01);
                    digital_fm_am.write(0b00);
                    psk_fsk_ask.write(0b00);

                    AD9834_Set_Freq(FREQ_0, 2e6 + AD9834_2_FREQ_ERROR);
                    SI5351_SetCLK2(2e6 + SI5351_FREQ_ERROR);
                }
            }
            else {      // FSK
                float32_t fsk_h = MAIN_measure_fsk_h(fs, rate);
                HMI_FSK_Transmit(rate, fsk_h);
                if (digital_fm_am.get_command() != 0b00 || psk_fsk_ask.get_command() != 0b01) {
                    AD9834_Set_Freq(FREQ_1, 3e6 + AD9834_3_FREQ_ERROR);
                    onekhz_twokhz.write(0b01);
                    digital_fm_am.write(0b00);
                    psk_fsk_ask.write(0b01);
                }
            }
        }
        else {      // ASK
            HMI_ASK_Transmit(rate);
            if (digital_fm_am.get_command() != 0b00 || psk_fsk_ask.get_command() != 0b10) {
                onekhz_twokhz.write(0b01);
                digital_fm_am.write(0b00);
                psk_fsk_ask.write(0b10);
            }
        }
    }
}

void MAIN_NEW_ANA_DIG_MODE() {
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
                HMI_ASK_Transmit(rate);
                if (digital_fm_am.get_command() != 0b00 || psk_fsk_ask.get_command() != 0b10) {
                    onekhz_twokhz.write(0b01);
                    digital_fm_am.write(0b00);
                    psk_fsk_ask.write(0b10);
                }
            }   // ASK
            else if (ask_fsk == 1) {    // FSK
                float32_t fsk_h = MAIN_measure_fsk_h(fs, rate);
                HMI_FSK_Transmit(rate, fsk_h);

                if (digital_fm_am.get_command() != 0b00 || psk_fsk_ask.get_command() != 0b01) {
                    AD9834_Set_Freq(FREQ_1, 3e6 + AD9834_3_FREQ_ERROR);
                    onekhz_twokhz.write(0b01);
                    digital_fm_am.write(0b00);
                    psk_fsk_ask.write(0b01);
                }
            }   // FSK
        }   // ASK/FSK
        else if (mod_type == 1) {   // PSK
            vca821_instance.set_gainvalue(50);  // max gen
            HMI_PSK_Transmit(rate);
            if (digital_fm_am.get_command() != 0b00 || psk_fsk_ask.get_command() != 0b00) {
                onekhz_twokhz.write(0b01);
                digital_fm_am.write(0b00);
                psk_fsk_ask.write(0b00);
//                        AD9854_SINSet(2e6, 4095);
//                        SI5351_SetCLK2(2e6);

                AD9834_Set_Freq(FREQ_0, 2e6 + AD9834_2_FREQ_ERROR);
                SI5351_SetCLK2(2e6 + SI5351_FREQ_ERROR);
            }
        }   // PSK
        else if (mod_type == 2) {   // ANALOG
            float32_t freq_use = 0;
            uint8_t is_am = MAIN_determine_analog_modulation(fs, freq_use);

            if (is_am == 1) {       // AM
                float32_t ma = MAIN_determine_ma(fs);
                HMI_AM_Transmit(freq_use, ma);
                if (digital_fm_am.get_command() != 0b10) {
                    onekhz_twokhz.write(0b01);
                    digital_fm_am.write(0b10);
                }
            }
            else if (is_am == 0) {      // FM
                float32_t delta_f = 0;
                float32_t mf = MAIN_determine_mf(fs, delta_f, 0.1, 1.5, 0.6);
                HMI_FM_Transmit(freq_use, mf);

                if (freq_use != fm_freq_use_record) {   // GPIO 1k 2k 窄带滤波器
                    fm_freq_use_record = freq_use;
                    if ( std::abs(freq_use - 1e3) < 500 ) {
                        onekhz_twokhz.write(0b00);
                    }
                    else if (std::abs(freq_use - 2e3) < 500 || std::abs(freq_use - 3e3) < 500) {
                        onekhz_twokhz.write(0b11);
                    }
                    else if (std::abs(freq_use - 4e3) < 500 || std::abs(freq_use - 5e3) < 500) {
                        onekhz_twokhz.write(0b10);
                    }
                    else {
                        onekhz_twokhz.write(0b01);
                    }
                }
                if (digital_fm_am.get_command() != 0b01) {
                    // 窄带
                    AD9834_Set_Freq(FREQ_1, 3e6 + AD9834_3_FREQ_ERROR);
                    digital_fm_am.write(0b01);
                }
            }
            else if (is_am == 2) {
//                        vec.fft_print();    HAL_Delay(400);
                HMI_CW_Transmit();
            }
            MAIN_AGC_agc(vca821_instance);  // only analog should
        }   // ANALOG
    }
}

void MAIN_NEW_SPECTRUM() {
    double receive_num = 0;
    HMI_Receive_num(receive_num);
    if (std::abs(receive_num - 1) < 1e-3) {
        float32_t fs = 320e3;
        ADC1_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, (uint32_t)fs);  // 联调的时候去掉
        Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
                      (float32_t *)fft_mag, SAMPLE);
        vec.fft("hann");


        HMI_Refresh(5);
        float32_t spetrum_max = fft_mag[vec.find_k_index(0)];
        HMI_LINE_Transmit((float32_t *) fft_mag, 2048, 2048, spetrum_max);
    }
}

