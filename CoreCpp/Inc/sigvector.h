//
// Created by outline on 2023/6/29.
//

// 2023/7/28 add flattop window v 1.2
// 2023/8/1 add point dtft mag v 1.3

#ifndef _SIGVECTOR_H
#define _SIGVECTOR_H

#include <iostream>
#include <string>
#include <vector>
#include "main_cpp.h"
#include <algorithm>
#include <numeric>

template <class T>
class Sigvector {
public:
    Sigvector(): data(nullptr), len(0), f_data(nullptr), fft_mag(nullptr) {};
    Sigvector(T* sigarray, uint32_t length) : data(sigarray), f_data(nullptr), fft_mag(nullptr), len(length) {}
    Sigvector(T* sigarray, float32_t *fft_data_, float32_t *fft_mag_,
              uint32_t length): data(sigarray), f_data(fft_data_), fft_mag(fft_mag_), len(length) {}

    void print();
    void fft_print(bool istring = true);
    float32_t max();
    float32_t max(uint32_t *index);
    float32_t max(uint32_t block_num);
    float32_t min();
    float32_t min(uint32_t *index);
    float32_t min(uint32_t block_num);
    float32_t vpp();
    float32_t vpp(uint32_t block_num);
    float32_t mean();


    void fft_input(float32_t *fft_data, float32_t *fft_mag);    // 为fft提供存储的数组，初始化添加了这里就不用添加了

    void fft(const std::string &win = "rec", uint8_t center_flag = 1);  // just call this function to fft
    void fft_sort_index();

    uint32_t find_k_index(uint32_t k) { return (uint32_t)fft_mag[len / 2 + k]; }    // 找到最大值所在的下标
    float32_t cal_freq(uint32_t index, float32_t f_s,  uint32_t side_num = 3);
    float32_t cal_freq(float32_t f_s, uint32_t side_num = 3);
    float32_t fft_vpp(uint32_t index, uint32_t side_num = 3);
    float32_t fft_vpp();

    void find_peaks(uint32_t min_peak_distance = 20,
                    float32_t threshold_l = 1, float32_t threshold_h = 0,
                    float32_t height_l = 2e5, float32_t height_h = 1e8,
                    float32_t plateau_size_l = 1, float32_t plateau_size_h = 0);
    uint32_t select_peaks(float32_t height_l_factor = 0.02, float32_t height_h_factor = 1.5, float32_t min_peak_distance_factor = 0.1);

    uint32_t peaks(uint32_t k = 0, uint8_t order = 0); // default return max peak's index
    [[nodiscard]] uint32_t peaks_len() const { return len_peaks; }
    void peaks_print();

    uint8_t judge_waveform(float32_t freq, float32_t fs);

    float32_t point_dtft_mag(float32_t w);
    float32_t cal_freq_dtft(float32_t fs, uint32_t iter_times = 5);


protected:
    T *data;
    float32_t *f_data;
    float32_t *fft_mag;
    uint32_t len;

    uint32_t len_peaks = 0;

    void fft_win(const std::string &win, uint8_t center_flag);         // 预处理，加窗或中心化
    void fft_();     // 直接做fft
};


/**
 * print wave in time domain.
 */
template <class T>
void Sigvector<T>::print() {
    for (uint32_t i = 0; i < len; ++i) {
        i == len - 1 ? printf("%d", data[i]) : printf("%d, ", data[i]);
    }
}

/**
 * print spectrum.
 */
template <class T>
void Sigvector<T>::fft_print(bool istring) {
    auto transmit_function = [&](uint32_t i) -> void {
        if (istring) {
            i == (len / 2) - 1 ? printf("%d", (int)fft_mag[i]) : printf("%d, ", (int)fft_mag[i]);
        }
        else {
            HAL_UART_Transmit(&huart1, (uint8_t *)&(fft_mag[i]), 4, 200);
        }
    };
    for (uint32_t i = 0; i < len / 2; ++i) {
        transmit_function(i);
//        i == (len / 2) - 1 ? printf("%d", (int)fft_mag[i]) : printf("%d, ", (int)fft_mag[i]);
    }
}

/**
 * find max element in time domain.
 * @return the max value
 */
template <class T>
float32_t Sigvector<T>::max() {
    uint32_t index;
    return max(&index);
}
/**
 * find max element and its index in time domain.
 * @param index index of max element
 * @return the max value
 */
template <class T>
float32_t Sigvector<T>::max(uint32_t *index) {
    double max_result = 0;
    uint32_t index_result = 0;
    for (uint32_t i = 0; i < len; ++i) {
        if ((float32_t)data[i] > max_result) {
            max_result = (float32_t)data[i];
            index_result = i;
        }
    }
    *index = index_result;
    return max_result;
}

/**
 * divide the wave into blocks of number block_num,
 * find every max element in each block, and return
 * the result with their average / rms / median.
 * @param block_num number of blocks
 * @return avg / rms / median
 */
template <class T>
float32_t Sigvector<T>::max(uint32_t block_num) {
    uint32_t block_size = len / block_num;
    float32_t block_max[20] = {0};    // 分块数最大不能超过20
    float32_t result = 0;
    for (uint32_t i = 0; i < block_num; ++i) {
        for (uint32_t j = i; j < i + block_size && j < len; ++j) {
            if ((float32_t)data[j] > block_max[i]) {
                block_max[i] = (float32_t)data[j];
            }
        }
    }
//    arm_mean_f32(block_max, block_num, &result);  // 平均值
    arm_rms_f32(block_max, block_num, &result);     // 均方根


    return result;
}

template <class T>
float32_t Sigvector<T>::min() {
    uint32_t index;
    return min(&index);
}
template <class T>
float32_t Sigvector<T>::min(uint32_t *index) {
    double min_result = 100000;
    uint32_t index_result = 0;
    for (uint32_t i = 0; i < len; ++i) {
        if ((float32_t)data[i] < min_result) {
            min_result = (float32_t)data[i];
            index_result = i;
        }
    }
    *index = index_result;
    return min_result;
}

template <class T>
float32_t Sigvector<T>::min(uint32_t block_num) {
    uint32_t block_size = len / block_num;
    float32_t block_min[20] = {0};    // 分块数最大不能超过20
    float32_t result = 0;
    for (uint32_t i = 0; i < block_num; ++i) {
        block_min[i] = 100000;
        for (uint32_t j = i; j < i + block_size && j < len; ++j) {
            if ((float32_t)data[j] < block_min[i]) {
                block_min[i] = (float32_t)data[j];
            }
        }
    }
    arm_mean_f32(block_min, block_num, &result);
    return result;
}

/**
 * max - min to find vpp
 * @return vpp
 */
template <class T>
float32_t Sigvector<T>::vpp() {
    return max() - min();
}

/**
 * divide blocks
 * @param block_num
 * @return max - min after dividing blocks
 */
template <class T>
float32_t Sigvector<T>::vpp(uint32_t block_num) {
    return max(block_num) - min(block_num);
}

/**
 * add f_data (float of adc_value) and fft_mag (result of rfft)
 * if you dont pass the array when initializing.
 * @param f_data_ float of adc_value, just an array, class will cal this.
 * @param fft_mag_ resutl of rfft, just an array, class will cal this.
 */
template <class T>
void Sigvector<T>::fft_input(float32_t *f_data_, float32_t *fft_mag_) {
    f_data = f_data_;
    fft_mag = fft_mag_;
}

/**
 * add win after you instance f_data and f_mag are not empty and centering input or not.
 * @param win type of win, which you can choose hann, hamming, rec and so on.
 * @param center_flag ture if you need center, vice versa.
 */
template <class T>
void Sigvector<T>::fft_win(const std::string &win, uint8_t center_flag) {  // 还没有选择窗的功能，默认为汉明窗
    float32_t mean_value = center_flag == 1 ? mean() : 0;

    if (win == "hann") {
        for (uint32_t i = 0; i < len; ++i) {
            f_data[i] = (float32_t)data[i] - mean_value;
            f_data[i] *= 0.5 * (1 - arm_cos_f32(2 * PI * (float32_t)i / (float32_t)(len - 1)));   // win
        }
    }
    else if (win == "hamming") {
        for (uint32_t i = 0; i < len; ++i) {
            f_data[i] = (float32_t)data[i] - mean_value;
            f_data[i] *= 0.54 - 0.46 * cos(2 * PI * (float32_t)i / (float32_t)(len - 1));;   // win
        }
    }
    else if (win == "flattop") {
        float32_t a0, a1, a2, a3, a4, k, n;
        a0 = 0.21557895;
        a1 = 0.41663158;
        a2 = 0.277263158;
        a3 = 0.083578947;
        a4 = 0.006947368;
        k = 1 / ((float32_t)len - 1);
        for (uint32_t i = 0; i < len; ++i) {
            f_data[i] = (float32_t)data[i] - mean_value;
            f_data[i] *= a0 - a1 * arm_cos_f32(2 * PI * (float32_t)i * k)
                            + a2 * arm_cos_f32(4 * PI * (float32_t)i * k)
                            - a3 * arm_cos_f32(6 * PI * (float32_t)i * k)
                            + a4 * arm_cos_f32(8 * PI * (float32_t)i * k);
        }
    }
    else {
        for (uint32_t i = 0; i < len; ++i) {
            f_data[i] = (float32_t)data[i] - mean_value;
        }
    }
}

/**
 * fft after 1. you input(or initialize) f_data and f_mag;
 * 2. add win or center.
 */
template <class T>
void Sigvector<T>::fft_() {
    arm_rfft_fast_instance_f32 rfft_s;
    arm_rfft_fast_init_f32(&rfft_s, len);

    arm_rfft_fast_f32(&rfft_s, f_data, fft_mag, 0); // 正变换，源数组会被改变

    arm_cmplx_mag_f32(fft_mag, fft_mag, len);
    fft_sort_index();       // 后面len / 2个点为坐标
}

/**
 * call fft_win and fft in one function, that means
 * you can call this method instead of above two in order.
 * @param win the same as fft_win.
 * @param center_flag the same as fft_win.
 */
template <class T>
void Sigvector<T>::fft(const std::string &win, uint8_t center_flag) {
    fft_win(win, center_flag);
    fft_();
    fft_win(win, center_flag);  // 恢复fft后被改变的原数组
}

/**
 * the len / 2 elements after fft_mag is used to indicate the sorted index by fft_mag.
 */
template <class T>
void Sigvector<T>::fft_sort_index() {
    std::iota(fft_mag + len / 2, fft_mag + len, 0);
    std::sort( fft_mag + len / 2, fft_mag + len,
               [&](size_t index_1, size_t index_2) { return fft_mag[index_1] > fft_mag[index_2]; });
}

/**
 * find the mean of adc_value.
 * @return mean of adc_value.
 */
template <class T>
float32_t Sigvector<T>::mean() {
    float32_t result = 0;
    for (uint32_t i = 0; i < len; ++i) {
        result += (float32_t)data[i];
    }
    return result / (float32_t)len;
}

/**
 * estimate freq using mean of PSD according to the index you pass or default max element index.
 * @param f_s sample rate
 * @param index default find max index or you give the index
 * @return Estimates of freq.
 */
template <class T>
float32_t Sigvector<T>::cal_freq(uint32_t index, float32_t f_s, uint32_t side_num) {
    float32_t temp_sum = fft_mag[index] * fft_mag[index];
    float32_t result = (float32_t)index * fft_mag[index] * fft_mag[index];
    uint32_t power_sidewin = index < side_num ? index : ( (len / 2 - index) < side_num ? len / 2 - index : side_num );
    for (uint32_t i = index - power_sidewin; i < index + power_sidewin; ++i) {
        if (i == index) {
            continue;
        }
        result += (float32_t)i * fft_mag[i] * fft_mag[i];
        temp_sum += fft_mag[i] * fft_mag[i];
    }
    return (result / temp_sum) * f_s / (float32_t)len;
}

template <class T>
float32_t Sigvector<T>::cal_freq(float32_t f_s, uint32_t side_num) {
    uint32_t index = find_k_index(0);
    return cal_freq(index, f_s, side_num);
}

/**
 * cal vpp by rms of discrete PSD
 * @param index default find max index or you give the index
 * @return estimates of vpp
 */
template <class T>
float32_t Sigvector<T>::fft_vpp(uint32_t index, uint32_t side_num) {
    float32_t temp_sum = 0;
    float32_t result = 0;
    uint32_t power_sidewin = index < side_num ? index : ( (len / 2 - index) < side_num ? len / 2 - index : side_num );
    for (uint32_t i = index - power_sidewin; i < index + power_sidewin; ++i) {
        temp_sum += fft_mag[i] * fft_mag[i];
    }
    arm_sqrt_f32(temp_sum, &result);
    return result;
}

template <class T>
float32_t Sigvector<T>::fft_vpp() {
    uint32_t index = find_k_index(0);
    return fft_vpp(index);
}

/**
 * find all the peaks in spectrum, the peaks are met with condition you input.
 * @param min_peak_distance from top to bottom, remove the lower peaks whose horizontal distance with
 *                          higher peak is less than the arg.
 * @param threshold_l the peak's neighbors which horizontal distance is 1. the vertical distance is higher than threshold_l.
 *                      [0, 1, 0] is n but [0, 2, 0] is y if threshold_l = 1
 * @param threshold_h
 * @param height_l limit the height of peaks
 * @param height_h
 * @param plateau_size_l
 * @param plateau_size_h
 * the method also help you sort the peaks according to the height of value in fft + len / 2 + len / 4 + len / 8
 */
template <class T>
void Sigvector<T>::find_peaks(uint32_t min_peak_distance,
                              float32_t threshold_l, float32_t threshold_h,
                              float32_t height_l, float32_t height_h,
                              float32_t plateau_size_l, float32_t plateau_size_h) {
    float32_t *x = fft_mag;
    uint32_t len_x = len / 2;
    len_peaks = 0;  // recall find_peaks should be reset len peaks.
    float32_t *peaks = fft_mag + len_x + len / 4;
    for (uint32_t i = 1; i < len / 2 - 1 && len_peaks < len / 4; ++i) {
        if (x[i - 1] < x[i]) {
            uint32_t iahead = i + 1;
            while (iahead < len_x - 1 && x[iahead] == x[i]) {
                ++iahead;
            }

            if (x[iahead] < x[i]) {
                bool peak_flag = true;

                if (plateau_size_l <= plateau_size_h) {     // 这个不常用，故没有测试
                    uint32_t current_plateau_size = iahead - i;

                    if ((float32_t)current_plateau_size < plateau_size_l ||
                        (float32_t)current_plateau_size > plateau_size_h) {

                        peak_flag = false;
                    }
                }

                if (height_l <= height_h) {

                    uint32_t current_peak_index = (i + iahead - 1) / 2;	// choose median

                    if (x[current_peak_index] < height_l || x[current_peak_index] > height_h) {

                        peak_flag = false;
                    }
                }

                if (threshold_l <= threshold_h) {

                    uint32_t current_peak_index = (i + iahead - 1) / 2;

                    if (std::min(x[current_peak_index] - x[current_peak_index - 1],
                                 x[current_peak_index] - x[current_peak_index + 1]) < threshold_l
                        || std::max(x[current_peak_index] - x[current_peak_index - 1],
                                    x[current_peak_index] - x[current_peak_index + 1]) > threshold_h) {

                        peak_flag = false;
                    }
                }
                if (peak_flag) {
                    peaks[len_peaks++] = (float32_t)(uint32_t)( (i + iahead - 1) / 2 );
                }
                i = iahead;
            }
        }
    }
    // sort peaks index
    float32_t *peaks_sorted = peaks + len / 8;
    arm_copy_f32(peaks, peaks_sorted, len_peaks);
    std::sort(peaks_sorted, peaks_sorted + len_peaks,
              [&](uint32_t index1, uint32_t index2) { return (x[index1] > x[index2]); });

    // min peak distance
    if (min_peak_distance > 1) {    // only mpd = 2, it could work
        for (uint32_t i = 0; i < len_peaks; ++i) {
            uint32_t j = std::find(peaks, peaks + len_peaks, peaks_sorted[i]) - peaks;
            for (int k = (int)(j) - 1; k >= 0 && j != len_peaks; --k) {
                if (peaks[j] - peaks[k] < (float32_t)min_peak_distance || peaks[k] < 0) { // 刚好包含不淘汰峰值
                    peaks[k] = -1;  // 淘汰
                }
                else {
                    break;
                }
            }
            for (int k = (int)(j + 1); k < len_peaks; ++k) {
                if (peaks[k] - peaks[j] < (float32_t)min_peak_distance || peaks[k] < 0) {
                    peaks[k] = -1;
                }
                else {
                    break;
                }
            }

        }
        auto peaks_end = std::remove(peaks, peaks + len_peaks, -1);
        len_peaks = peaks_end - peaks;
        arm_copy_f32(peaks, peaks_sorted, len_peaks);
        std::sort(peaks_sorted, peaks_sorted + len_peaks,
                  [&](uint32_t index1, uint32_t index2) { return (x[index1] > x[index2]); });

    }


}

/**
 * return the peak you want. if order is 1, than k means the order of freq
 * if order is 0, k means the height order
 * @param k
 * @param order
 * @return
 */
template <class T>
uint32_t Sigvector<T>::peaks(uint32_t k, uint8_t order) {
    uint32_t result = 0;
    if (order == 0) {
        result = std::round((uint32_t)fft_mag[len / 2 + len / 4 + len / 8 + k]);    // sort peaks
    }
    else {
        result = std::round((uint32_t)fft_mag[len / 2 + len / 4 + k]);  // freq
    }
    return result;
}

template <class T>
void Sigvector<T>::peaks_print() {
    float32_t *peaks = fft_mag + len / 2 + len / 4;
    printf("\n");
    for (uint32_t i = 0; i < len_peaks; ++i) {
        printf("%d ", (int)peaks[i]);
    }
    printf("len peaks: %d\n", (int)len_peaks);
}

/**
 * first find suitable height_l and height_h
 * find suitable min_peak_distance according to the horizontal distance between peak.
 * @param height_l_factor
 * @param height_h_factor
 * @return the distance between peak, 0 means no peaks
 */
template <class T>
uint32_t Sigvector<T>::select_peaks(float32_t height_l_factor, float32_t height_h_factor, float32_t min_peak_distance_factor) {
    float32_t *peaks = fft_mag + len / 2 + len / 4;
    float32_t *order_spectrum = fft_mag + len / 2;
    float32_t spectrum_max = fft_mag[(uint32_t)std::round(order_spectrum[0])];

    float32_t height_l = (float32_t)height_l_factor * spectrum_max;      // 0.02
    float32_t height_h = (float32_t)height_h_factor * spectrum_max;       // 1.5

    find_peaks(2, 1, 0,
               height_l, height_h, 1, 0);    // 1k低频，12k频偏的参数
//    peaks_print();
    if (len_peaks < 3 || len_peaks > 50) {   // 没有峰值
        return 0;
    }

    float32_t diff_peaks[50] = {0};     // I don't know how many peaks, maybe 50 is max;
    for (uint32_t i = 0; i < len_peaks - 1; ++i) {
        diff_peaks[i] = peaks[i + 1] - peaks[i];
    }
    uint32_t len_diff_peaks = len_peaks - 1;
    std::sort(diff_peaks, diff_peaks + len_diff_peaks);
    auto min_peak_distance = std::ceil((float32_t)(diff_peaks[len_diff_peaks / 2] * min_peak_distance_factor));  // 峰间距离的一半为阈值
    find_peaks((uint32_t)min_peak_distance, 1, 0,
               height_l, height_h, 1, 0);    // 1k低频，12k频偏的参数
    return (uint32_t)diff_peaks[len_diff_peaks / 2];
}

template <class T>
uint8_t Sigvector<T>::judge_waveform(float32_t freq, float32_t fs) {
    float32_t peaks_value[3] = {0};
    float32_t height_l = fft_mag[find_k_index(0)] / (float32_t)35;
    auto min_peak_distance = (uint32_t)(2 * freq * (float32_t)len * 0.75 / fs);
    find_peaks(min_peak_distance, 1, 0,
               height_l, 1e8, 1, 0);
    if (len_peaks == 1) {
        return 0;
    }
    float32_t *peaks_index = fft_mag + len / 2 + len / 4;
    for (int i = 0; i < 3; ++i) {
        auto temp_index = (uint32_t)peaks_index[i];
        peaks_value[i] = fft_mag[temp_index];
    }
    float32_t peaksvalue_divide[2] = {0};
    for (int i = 0; i < 2; ++i) {
        peaksvalue_divide[i] = peaks_value[0] / peaks_value[i + 1];
    }
    double error_triangular = 0;
    double error_rectangle = 0;
    error_triangular = std::abs(9 - peaksvalue_divide[0]) / 9 + std::abs(25 - peaksvalue_divide[1]) / 25;
    error_rectangle = std::abs(3 - peaksvalue_divide[0]) / 3 + std::abs(5 - peaksvalue_divide[1]) / 5;
    if (error_triangular < error_rectangle) {
        return 2;       // tri
    }
    else {
        return 1;       // rec
    }
}

/**
 * cal the mag of dtft of adc_value with digital freq w which range is [0 : 2 * pi].
 * @tparam T
 * @param w digital freq
 * @return mag value of X(exp(jw))
 */
template<class T>
float32_t Sigvector<T>::point_dtft_mag(float32_t w) {
    float32_t result = 0;
    float32_t real_accumulate = 0;
    float32_t imag_accumulate = 0;
    for (uint32_t i = 0; i < len; ++i) {
        float32_t twiddle_factor_real = arm_cos_f32((float32_t)i * w);
        float32_t twiddle_factor_imag = -arm_sin_f32((float32_t)i * w);
        real_accumulate += twiddle_factor_real * f_data[i];
        imag_accumulate += twiddle_factor_imag * f_data[i];
    }

    return std::sqrt(real_accumulate * real_accumulate + imag_accumulate * imag_accumulate);
//    arm_sqrt_f32(real_accumulate * real_accumulate + imag_accumulate * imag_accumulate, &result);
//    printf("111");
//    return result;
}


template<class T>
float32_t Sigvector<T>::cal_freq_dtft(float32_t fs, uint32_t iter_times) {
    uint32_t higher_index = find_k_index(0);
    uint32_t lower_index = fft_mag[higher_index + 1] > fft_mag[higher_index - 1] ?
                                higher_index + 1 : higher_index - 1;

    float32_t higher_w = (float32_t)higher_index * 2 * PI / len;
    float32_t lower_w = (float32_t)lower_index * 2 * PI / len;
    float32_t higher_val = fft_mag[higher_index];
    float32_t lower_val = fft_mag[lower_index];

    float32_t mid_w = 0;
    float32_t mid_val = 0;
    for (uint32_t i = 0; i < iter_times; ++i) {
        mid_w = (higher_w + lower_w) / 2;
        mid_val = point_dtft_mag(mid_w);
        if (mid_val > higher_val) {
            lower_val = higher_val;
            lower_w = higher_w;
            higher_val = mid_val;
            higher_w = mid_w;
        }
        else if (mid_val > lower_val) {
            lower_val = mid_val;
            lower_w = mid_w;
        }
    }
    return mid_w * fs / (2 * PI);
}




#endif //_SIGVECTOR_H
