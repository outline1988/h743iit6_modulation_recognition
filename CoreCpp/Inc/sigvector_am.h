//
// Created by outline on 2023/6/30.
//

#ifndef _SIGVECTOR_AM_H
#define _SIGVECTOR_AM_H

#include "sigvector.h"
class Sigvector_am: public Sigvector<uint16_t> {
public:
    using Sigvector::Sigvector;     // 继承构造函数

    float32_t cal_ma();

private:

};


#endif //_SIGVECTOR_AM_H
