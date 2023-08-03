
//
// Created by outline on .
//

#include "main_cpp.h"
#include "adc_capture.h"
#include "sigvector.h"
#include "sigvector_fm.h"
#include "sigvector_am.h"
#include "hmi_display.h"

#define SAMPLE (4096)
__attribute__((section ("._dma_buffer"))) volatile uint16_t adc_value[SAMPLE * 4];
__attribute__((section ("._dma_buffer"))) volatile float32_t float_data[SAMPLE * 4];
__attribute__((section ("._dma_buffer"))) volatile float32_t fft_mag[SAMPLE * 4];
//__attribute__((section ("._dma_buffer"))) volatile float32_t measure_h_adc_value[SAMPLE];

float32_t MAIN_determine_fc();
float32_t MAIN_determine_fs(float32_t fc);
float32_t MAIN_determine_ma(float32_t fs);
float32_t MAIN_determine_mf(float32_t fs, float32_t &delta_f);
uint8_t MAIN_determine_analog_modulation(float32_t fs, float32_t &freq_use);
void MAIN_determine_dds(float32_t fc);
uint8_t MAIN_determine_psk_rate(Sigvector<uint16_t> &vec, float32_t fs, float32_t &rate);
uint8_t MAIN_determine_fsk_ask(const float32_t *f_data);
uint8_t MAIN_determine_digital_modulation(float32_t fs, float32_t &rate);
float32_t MAIN_measure_fsk_h(float32_t rate);


int main_cpp_analog_result() {
    RETARGET_Init(&huart1);
    ADC_CAPTURE_Init(&hadc1, &htim8);   // 带同采样PC4
    ADC_EXTCAPTURE_Init(&hadc3, &hi2c3);
    AD9910_Init();

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
    ADC_CAPTURE_Init(&hadc1, &htim8);   // 带通采样PC4
    ADC_EXTCAPTURE_Init(&hadc3, &hi2c3);

    while (true) {
//        float32_t measure_h_fs = 4.5e6;
        float32_t a = MAIN_measure_fsk_h(1000);
        printf("%d", (int)a);
//        uint32_t digital_type = MAIN_determine_digital_modulation(fs, rate);



//        if (digital_type == 0) {
//            printf("ASK rate: %d", (int)rate);
//        }
//        else if (digital_type == 1) {
//            printf("FSK rate: %d", (int)rate);
//        }
//        else if(digital_type == 2) {
//            printf("PSK rate: %d", (int)rate);
//        }
        printf("\n\n");
        HAL_Delay(500);
    }
    return 0;
}

int main_cpp()  {
    RETARGET_Init(&huart1);
    ADC_CAPTURE_Init(&hadc1, &htim8);   // 带通采样PC4
    ADC_EXTCAPTURE_Init(&hadc3, &hi2c3);
    HMI_Init(&huart3);
    uint8_t mode_select = 0;
    while (true) {
//        printf("hello, world\n");
        HMI_Mode(mode_select);
        if (mode_select == 1) {     // analog
            double receive_num = 0;
            HMI_Receive_num(receive_num);
            if (std::abs(receive_num - 1) < 1e-3) {
                float32_t fs = 320e3;   // 240e3
                float32_t freq_use = 0;
                uint8_t is_am = MAIN_determine_analog_modulation(fs, freq_use);

                char msg[50] = {0};
                if (is_am == 1) {
                    float32_t ma = MAIN_determine_ma(fs);
                    HMI_TXT_Transmit("ANALOG MODULATION: AM", 0);
                    sprintf(msg, "MA: %d", (int)std::round(ma * 1000));
                    HMI_TXT_Transmit(msg, 1);
                    sprintf(msg, "FREQ: %d", (int)std::round(freq_use));
                    HMI_TXT_Transmit(msg, 2);
                }
                else if (is_am == 0) {
                    float32_t delta_f = 0;
                    float32_t mf = MAIN_determine_mf(fs, delta_f);
                    HMI_TXT_Transmit("ANALOG MODULATION: FM", 0);
                    sprintf(msg, "MF: %d    DEV: %d", (int)std::round(mf * 1000), (int)std::round(freq_use * mf));
                    HMI_TXT_Transmit(msg, 1);

                    sprintf(msg, "FREQ: %d", (int)std::round(freq_use));
                    HMI_TXT_Transmit(msg, 2);
//                    printf("mf: %d    delta_f: %d", int(mf * 100), int(delta_f));
                }
                else if (is_am == 2) {
                    HMI_TXT_Transmit("CW", 0);
                    HMI_TXT_Transmit("CW", 1);
                    HMI_TXT_Transmit("CW", 2);
                }
            }
        }
        else if (mode_select == 2) {     // digital
            double receive_num = 0;
            HMI_Receive_num(receive_num);
            if (std::abs(receive_num - 1) < 1e-3) {
                float32_t fs = 320e3;
                float32_t rate = 0;
                uint32_t digital_type = MAIN_determine_digital_modulation(fs, rate);

                char msg[50] = {0};
                if (digital_type == 0) {
                    HMI_TXT_Transmit("DIGITAL MODULATION: ASK", 0);
                    sprintf(msg, "rate: %d", (int)std::round(rate));
                    HMI_TXT_Transmit(msg, 1);
                    HMI_TXT_Transmit(" ", 2);
                }
                else if (digital_type == 1) {
                    // 这里在测量h
                    float32_t fsk_h = MAIN_measure_fsk_h(rate);
                    HMI_TXT_Transmit("DIGITAL MODULATION: FSK", 0);
                    sprintf(msg, "rate: %d", (int)std::round(rate));
                    HMI_TXT_Transmit(msg, 1);
                    sprintf(msg, "h: %d", (int)std::round(fsk_h * 1000));
                    HMI_TXT_Transmit(msg, 2);
                }
                else if (digital_type == 2) {
                    HMI_TXT_Transmit("DIGITAL MODULATION: PSK", 0);
                    sprintf(msg, "rate: %d", (int)std::round(rate));
                    HMI_TXT_Transmit(msg, 1);
                    HMI_TXT_Transmit(" ", 2);
                }

            }
        }

//        else if (mode_select == 3) {
//            double receive_num = 0;
//            HMI_Receive_num(receive_num);
//            if (std::abs(receive_num - 1) < 1e-3) {
//                ADC3_EXTCAPTURE_Capture((uint16_t *)adc_value, SAMPLE, 30e3);
//                Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
//                              (float32_t *)fft_mag, SAMPLE);
//                vec.fft("hann");
//
//
//                HMI_Refresh(3);
//                HMI_LINE_Transmit((float32_t *)fft_mag, 2048, 2048, 1e6);
//            }
//        }
        HAL_Delay(100);
    }
    return 0;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC3) {
        ADC3_EXTCAPTURE_Callback();
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

float32_t MAIN_determine_fc() {
//    uint32_t fs_remainder[3] = {(uint32_t)3.870968e6, (uint32_t)3.076923e6, (uint32_t)2.580645e6};
//    uint32_t fs_remainder[3] = {(uint32_t)9.230769e6, (uint32_t)8.888889e6, (uint32_t)8.571429e6};  // danger
    uint32_t fs_remainder[3] = {(uint32_t)7.272727e6, (uint32_t)7.058823e6, (uint32_t)6.857143e6};

//    uint32_t fs_remainder[3] = {(uint32_t)10e3, (uint32_t)20e3, (uint32_t)40e3};
//    uint32_t fs_remainder[2] = {(uint32_t)9.230769e6, (uint32_t)8.571429e6};
//    uint32_t fs_remainder[2] = {(uint32_t)9.230769e6, (uint32_t)8.888889e6};
//    uint32_t fs_remainder[2] = {(uint32_t)7.058823e6, (uint32_t)8.888889e6};
    float32_t f_remainder[3] = {0};
    for (int i = 0; i < 3; ++i) {
        ADC2_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, fs_remainder[i]);
        Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
                      (float32_t *)fft_mag, SAMPLE);
        vec.fft("hann");
//        vec.fft_print();
//        printf("\n");
        float32_t f_measure = vec.cal_freq((float32_t)fs_remainder[i], 0);
        f_remainder[i] = f_measure;
    }

    const uint32_t f_perhaps_num = 10;
    float32_t f1_perhaps[f_perhaps_num * 2] = {0};
    float32_t f2_perhaps[f_perhaps_num * 2] = {0};
    float32_t f3_perhaps[f_perhaps_num * 2] = {0};
    for (int i = 0; i < f_perhaps_num; ++i) {
        f1_perhaps[2 * i] = f_remainder[0] + (float32_t)(i) * (float32_t)fs_remainder[0];
        f1_perhaps[2 * i + 1] = -f_remainder[0] + (float32_t)(i + 1) * (float32_t)fs_remainder[0];
        f2_perhaps[2 * i] = f_remainder[1] + (float32_t)(i) * (float32_t)fs_remainder[1];
        f2_perhaps[2 * i + 1] = -f_remainder[1] + (float32_t)(i + 1) * (float32_t)fs_remainder[1];
        f3_perhaps[2 * i] = f_remainder[2] + (float32_t)(i) * (float32_t)fs_remainder[2];
        f3_perhaps[2 * i + 1] = -f_remainder[2] + (float32_t)(i + 1) * (float32_t)fs_remainder[2];
    }

    float32_t error_min = 1e16;
    float32_t f1_result = 0;
    float32_t f2_result = 0;
    float32_t f3_result = 0;
    for (int i = 0; i < f_perhaps_num * 2; ++i) {
        for (int j = 0; j < f_perhaps_num * 2; ++j) {
            for (int k = 0; k < f_perhaps_num * 2; ++k) {

//                float32_t error = std::abs(f1_perhaps[i] - f2_perhaps[j]);
                float32_t x = f1_perhaps[i], y = f2_perhaps[j], z = f3_perhaps[k];
                float32_t error = x * x + y * y + z * z - x * y - x * z - y * z;
                if ((f1_perhaps[i] > 9.5e6 && f1_perhaps[i] < 30.5e6) &&
                    (f2_perhaps[j] > 9.5e6 && f2_perhaps[j] < 30.5e6) &&
                    (f3_perhaps[k] > 9.5e6 && f3_perhaps[k] < 30.5e6) &&
                    error < error_min) {
                    error_min = error;
                    f1_result = f1_perhaps[i];
                    f2_result = f2_perhaps[j];
                    f3_result = f3_perhaps[k];
                }
            }
        }
    }
    return (f1_result + f2_result + f3_result) / 3;
}

float32_t MAIN_determine_fs(float32_t fc) {
    const float32_t band = 130e3;
    const float32_t fs_ll = 400e3;
    const float32_t fs_hh = 800e3;
    uint32_t m_l = std::ceil((float32_t)((2 * fc - band) / (float32_t)fs_hh));
    uint32_t m_h = std::floor((float32_t)((2 * fc + band) / (float32_t)fs_ll));
    uint32_t m = (m_l + m_h) / 2;
    float32_t fs_l = (2 * fc + band) / (float32_t)(m + 1);
    float32_t fs_h = (2 * fc - band) / (float32_t)m;

    return (fs_l + fs_h) / (float32_t)2;
}

float32_t MAIN_determine_ma(float32_t fs) {
//    ADC1_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, (uint32_t)fs);  // 联调的时候去掉
    Sigvector_am vec((uint16_t *)adc_value, (float32_t *)float_data,
                     (float32_t *)fft_mag, SAMPLE);

//    vec.fft("hann");    // 联调的时候去掉

    vec.select_peaks(0.02, 1.5);

    float32_t ma = vec.cal_ma();
    return ma;
}

float32_t MAIN_determine_mf(float32_t fs, float32_t &delta_f) {
//    ADC1_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, (uint32_t)fs);  // 联调的时候去掉
    Sigvector_fm vec((uint16_t *)adc_value, (float32_t *)float_data,
                     (float32_t *)fft_mag, SAMPLE);

//    vec.fft("hann");    // 联调的时候去掉
//    vec.fft_print();
//    printf("\n");


    uint32_t omega_index = vec.select_peaks();     // find_peaks is needed cause len_peaks should be updated in Sigvector_fm class.


    float32_t mf = vec.cal_mf();
//    vec.peaks_print();
    delta_f = (float32_t)omega_index * fs * mf / SAMPLE;
    return mf;
}

/**
 *
 * @param fs
 * @return am return 1, fm return 0, no peaks 3
 */
uint8_t MAIN_determine_analog_modulation(float32_t fs, float32_t &freq_use) {
    ADC1_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, (uint32_t)fs);
    Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
                     (float32_t *)fft_mag, SAMPLE);

    vec.fft("hann");      // default rectangle win and center



    vec.select_peaks(0.02, 1.5);

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
    freq_use = freq_h - freq_l;


    uint32_t len_peaks = vec.peaks_len();   // am调制
    if (len_peaks > 2 && len_peaks < 6) {    // 3、4或5个极值点   // 下策
        return 1;
    }
    else if (len_peaks < 3) {      // 1个极点，单频
        return 2;
    }
    return 0;   // fm调制
}

void MAIN_determine_dds(float32_t fc) {
    const float32_t f_mixer_result = 5e6;
    float32_t f_mixer_local_temp = fc + f_mixer_result;
    static float32_t f_mixer_local = 50e6;
    if (std::abs(f_mixer_local - f_mixer_local_temp) > 0.8e6) {
        f_mixer_local = f_mixer_local_temp;
        AD9910_STone_Switch(16383, f_mixer_local, 0);
    }
}

/**
 * 通过频率间隔判断出PSK还是其他
 * @param vec
 * @param fs
 * @return 1表示PSK，0表示除PSK之外的
 */
uint8_t MAIN_determine_psk_rate(Sigvector<uint16_t> &vec, float32_t fs, float32_t &rate) {
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
//    printf("delta freq: %d\n", (int)delta_freq);
    if (delta_freq > 5.5e3) {   // large delta freq, must psk
        rate = delta_freq;
        return 1;
    }
    else {
        rate = delta_freq * 2;
        return 0;
    }
}

uint8_t MAIN_determine_fsk_ask(const float32_t *f_data) {
    float32_t max_time_val = 0;
    uint32_t max_time_index = 0;
    arm_max_f32(f_data, SAMPLE, &max_time_val, &max_time_index);
    float32_t accumulate_result = 0;
    for (uint32_t i = 0; i < SAMPLE; ++i) {
        accumulate_result += std::abs(f_data[i]) / max_time_val;
    }
//    printf("accumulate: %d", (int)accumulate_result);
    if (accumulate_result > 1000) {
        return 1;   // FSK
    }
    else {
        return 0;   // ASK
    }
}

/**
 * 返回数字调制的类型
 * @param fs
 * @param rate
 * @return 0: ASK   1: FSK      2: PSK      3: NO POSSIBLE
 */
uint8_t MAIN_determine_digital_modulation(float32_t fs, float32_t &rate) {
    ADC1_CAPTURE_Capture((uint16_t *)adc_value, SAMPLE, (uint32_t)fs);  // 联调的时候去掉
    Sigvector vec((uint16_t *)adc_value, (float32_t *)float_data,
                  (float32_t *)fft_mag, SAMPLE);

    vec.fft("hann");    // 联调的时候去掉
//        vec.print();
//    vec.print();

//        vec.fft_print();

    uint8_t mod_type = MAIN_determine_psk_rate(vec, fs, rate);
    if (mod_type == 0) {    // ASK 或 FSK
        uint8_t ask_fsk = MAIN_determine_fsk_ask((float32_t *)float_data);
        if (ask_fsk == 0) {
//            printf("ASK ");
            return 0;
        }
        else if (ask_fsk == 1) {
//            printf("FSK ");
            return 1;
        }
//        printf("rate: %d", (int)rate);
    }
    else if (mod_type == 1) {
//        printf("PSK rate: %d", (int)rate);
        return 2;
    }
    return 3;
}

float32_t MAIN_measure_fsk_h(float32_t rate) {
    float32_t measure_h_fs = 4.5e6;

    ADC3_EXTCAPTURE_Capture((uint16_t *)adc_value, SAMPLE, (uint32_t)measure_h_fs);
    Sigvector measure_h_vec((uint16_t *)adc_value, (float32_t *)float_data,
                            (float32_t *)fft_mag, SAMPLE);

    measure_h_vec.fft("hann");      // default rectangle win and center

    uint32_t max1_peaks_index = measure_h_vec.find_k_index(0);
    uint32_t max2_peaks_index = measure_h_vec.find_k_index(1);
//        float32_t measure_h_freq1 = (float32_t)max1_peaks_index * measure_h_fs / SAMPLE;
//        float32_t measure_h_freq2 = (float32_t)max2_peaks_index * measure_h_fs / SAMPLE;
    float32_t measure_h_freq1 = measure_h_vec.cal_freq(max1_peaks_index, measure_h_fs, 1);
    float32_t measure_h_freq2 = measure_h_vec.cal_freq(max2_peaks_index, measure_h_fs, 1);
    float32_t f_diff = std::abs(measure_h_freq1 - measure_h_freq2);
    return f_diff / rate;
}
