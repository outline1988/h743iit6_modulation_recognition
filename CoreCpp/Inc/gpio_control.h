//
// Created by outline on 2023/8/3.
//

#ifndef _GPIO_CONTROL_H
#define _GPIO_CONTROL_H
#include "main_cpp.h"

class Gpio_control {
public:
    Gpio_control(GPIO_TypeDef *GPIOx1_, uint16_t GPIO_Pin1_, GPIO_TypeDef *GPIOx0_, uint16_t GPIO_Pin0_):
        GPIOx1(GPIOx1_), GPIO_Pin1(GPIO_Pin1_), GPIOx0(GPIOx0_), GPIO_Pin0(GPIO_Pin0_) { write(0b00); }

    Gpio_control &write(uint8_t command);
    [[nodiscard]] uint8_t get_command() const { return current_command; };
private:
    GPIO_TypeDef *GPIOx1;
    uint16_t GPIO_Pin1;
    GPIO_TypeDef *GPIOx0;
    uint16_t GPIO_Pin0;
    uint8_t current_command = 0x00;
};

#endif //_GPIO_CONTROL_H
