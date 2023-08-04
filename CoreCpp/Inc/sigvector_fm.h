//
// Created by outline on 2023/6/30.
//

#ifndef _SIGVECTOR_FM_H
#define _SIGVECTOR_FM_H

#include "sigvector.h"
class Sigvector_fm: public Sigvector<uint16_t> {
public:
    using Sigvector::Sigvector;     // 继承构造函数

    float32_t cal_mf();
    float32_t cal_fsk_h(float32_t fs, float32_t rate);

    float32_t loss_function(const float32_t *bessel_value, uint32_t bessel_len, uint32_t i);
    uint32_t find_center_index(float32_t *index_array, uint32_t &len_index_array);


private:


    void fill_peak(float32_t *peak_index, uint32_t &len_peak_index);



};

#endif //_SIGVECTOR_FM_H
