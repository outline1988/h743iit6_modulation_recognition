//
// Created by outline on 2023/8/3.
//
#include "gpio_control.h"


Gpio_control &Gpio_control::write(uint8_t command) {
    current_command = command;
    if (command & 0b01) {
        HAL_GPIO_WritePin(GPIOx0, GPIO_Pin0, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_WritePin(GPIOx0, GPIO_Pin0, GPIO_PIN_RESET);
    }

    if (command & 0b10) {
        HAL_GPIO_WritePin(GPIOx1, GPIO_Pin1, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_WritePin(GPIOx1, GPIO_Pin1, GPIO_PIN_RESET);
    }
    return *this;
}
