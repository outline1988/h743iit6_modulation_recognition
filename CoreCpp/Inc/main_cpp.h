
//
// Created by outline on .
//

#ifndef _MAIN_CPP_H
#define _MAIN_CPP_H

#ifdef __cplusplus
extern "C" {
#endif

// C Standard Library Header
#include <string.h>

// HAL Library Header
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

// Other Library Header
#include "arm_math.h"    // must add this line after main.h

// Drivers of Mine Header
#include "retarget.h"
#include "SI5351.h"
#include "AD9910.h"
#include "FFTInc.h"
#include "AD9854.h"
#include "AD9834.h"

int main_cpp();

#ifdef __cplusplus
}
#endif

#endif //_MAIN_CPP_H

