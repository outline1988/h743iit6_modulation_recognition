//
// Created by outline on 2023/6/30.
//
#include "sigvector_am.h"

/**
 * 计算完了fft之后再来调用这个函数
 * @return
 */
float32_t Sigvector_am::cal_ma() {

    float32_t dsb_vpp[3] = {0};
    for(uint32_t i = 0, j = 0; i < 3; ++i) {
        uint32_t temp_index = peaks(i);
//        if (temp_index < (len / 10)) {
//            continue;
//        }
        dsb_vpp[j++] = fft_vpp(temp_index);
    }
    return (dsb_vpp[1] + dsb_vpp[2]) / dsb_vpp[0];
}
