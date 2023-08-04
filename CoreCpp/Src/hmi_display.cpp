//
// Created by outline on 2023/7/5.
//
#include "hmi_display.h"

UART_HandleTypeDef *__huart = nullptr;
uint8_t rx[10] = {0};
char msg[50] = {0};
char float_data_[50] = {0};

void HMI_Init(UART_HandleTypeDef *huart) {
    __huart = huart;
    HMI_Start();
}

void HMI_Start() {
    HAL_UART_Receive_IT(__huart, (uint8_t *)rx, 1);
}

/**
 * select receive num or mode
 * @return mode return true, num return false
 */
static bool HMI_IsMode() {
    return (rx[0] < 0xf0);
}

void HMI_Mode(uint8_t &mode_select) {    // just 8 bit t select mode
    if (HMI_IsMode()) {
        mode_select = rx[0];    // update mode_select
        return;
    }
    else {
        return;
    }
}

void HMI_Receive_num(double &num) {
    if (HMI_IsMode()) {
        return;
    }
    uint32_t result = 0;
    uint16_t size = rx[0] - 0xf0;
    for (uint16_t i = 0; i < size; ++i) {
        result += (uint32_t)(rx[i + 1]) << (8 * (i));
    }
    num = (double)result;
}

void HMI_Callback() {
    if (HMI_IsMode()) {
        HAL_UART_Receive_IT(__huart, (uint8_t *)rx, 1);
    }
    else {
        uint16_t size = rx[0] - 0xf0;
        HAL_UART_Receive(__huart, (uint8_t *)(rx + 1), size, 200);  // continue receiving size byte data
        HAL_UART_Receive_IT(__huart, (uint8_t *)rx, 1);
    }
}

void HMI_Transmit(char *msg_) {
    HAL_UART_Transmit(__huart, (uint8_t *)msg_, strlen(msg_), 500);
}

void HMI_TXT_Transmit(uint32_t uint32_num, uint32_t id) {
    sprintf(msg, "t%d.txt=\"%d\"" "\xff\xff\xff", (int)id, (int)(uint32_num));
    HMI_Transmit(msg);
}

void HMI_TXT_Transmit(const char *msg_, uint32_t id) {
    sprintf(msg, "t%d.txt=\"%s\"" "\xff\xff\xff", (int)id, msg_);
    HMI_Transmit(msg);
}

void HMI_TXT_FLOAT_Transmit(float32_t data, uint32_t id) {
    sprintf(msg, "t%d.txt=\"%d.%d\"" "\xff\xff\xff", (int)id, (int)data, (int)std::round( (data - (int)data) * 1000 ) );
    HMI_Transmit(msg);
}

void HMI_TXT_INT_Transmit(float32_t data, uint32_t id) {
    sprintf(msg, "t%d.txt=\"%d\"" "\xff\xff\xff", (int)id, (int)std::round(data));
    HMI_Transmit(msg);
}

void HMI_WAVE_Transmit(uint8_t val, uint8_t channel, uint32_t id) {
    sprintf(msg, "add s%d.id,%d,%d" "\xff\xff\xff", (int)id, (int)channel, (int)val);
    HMI_Transmit(msg);
}

void HMI_WAVE_Transmit(const uint8_t *val_array, uint32_t len, uint8_t channel, uint32_t id) {
    for (uint32_t i = 0; i < len; ++i) {
        HMI_WAVE_Transmit(val_array[i], channel, id);
    }
}

// 长就是控件中的长，高0-255
void HMI_WAVE_Transmit(const uint16_t *val_array, uint32_t len, uint16_t max, uint16_t min,
                       uint8_t channel, uint32_t id) {
    double k = 255.0 / (double)(max - min);
    uint8_t temp;
    for (uint32_t i = 0; i < len; ++i) {
        temp = (uint8_t)(k * val_array[i]);
        HMI_WAVE_Transmit(temp, channel, id);
    }
}

// 串口屏中 横轴x 800，纵轴y 480，原点左上角
// 本函数改为了横轴x 800，纵轴y 480，原点左下角
// 在上一行的基础上，原点改为50,50
// 此时x最大700   y最大380
const uint32_t LINE_X_MAX = 700;
const uint32_t LINE_Y_MAX = 380;
void HMI_LINE_Transmit(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint8_t colour) {
    x1 = x1 > LINE_X_MAX ? LINE_X_MAX : x1;
    x2 = x2 > LINE_X_MAX ? LINE_X_MAX : x2;
    y1 = y1 > LINE_Y_MAX ? LINE_Y_MAX : y1;
    y2 = y2 > LINE_Y_MAX ? LINE_Y_MAX : y2;
    const uint32_t x_zero = 50;
    const uint32_t y_zero = 50;
    sprintf(msg, "line %d,%d,%d,%d,%d" "\xff\xff\xff",
                (int)(x1 + x_zero), (int)(480 - y1 - y_zero), (int)(x2 + x_zero), (int)(480 - y2 - y_zero), colour);
    HMI_Transmit(msg);
}


void HMI_LINE_Transmit(const float32_t *array, uint32_t len, uint32_t len_max, float32_t val_max, uint8_t colour) {
    len = len > len_max ? len_max : len;    // 长度太长就截断不画后面的了
    if (len < LINE_X_MAX) {     // 点数不足以填满x轴，循环len。从数组转换至坐标
        double x_k =(double)LINE_X_MAX / len_max;
        double y_k = (double)LINE_Y_MAX / val_max;
        for (uint32_t i = 0; i < len - 1; ++i) {
            auto x = (uint32_t)std::round(i * x_k);
            auto y = (uint32_t)std::round(array[i] * y_k);
            auto x_next = (uint32_t)std::round((i + 1) * x_k);
            auto y_next = (uint32_t)std::round(array[i + 1] * y_k);
            HMI_LINE_Transmit(x, y, x_next, y_next, colour);
        }
    }
    else {  // LINE_X_MAX，循环LINE_X_MAX。x轴最近邻插值从坐标转回数组，y轴仍然从数组转换为坐标
        double x_k = len_max / (double)LINE_X_MAX;
        double y_k = (double)LINE_Y_MAX / val_max;
        auto loop_times = (uint32_t)std::floor(LINE_X_MAX * len / len_max) - 1;
        for (uint32_t i = 0; i < loop_times; ++i) {
            auto index = (uint32_t)std::round(i * x_k);     // 最近邻还原
            auto y = (uint32_t)std::round(array[index] * y_k);
            auto index_next = (uint32_t)std::round((i + 1) * x_k);
            auto y_next = (uint32_t)std::round(array[index_next] * y_k);
            HMI_LINE_Transmit(i, y, i + 1, y_next, colour);
        }
    }
}

// 一模一样的函数但是重载为uint16_t，虽然不优雅，但是不管了
void HMI_LINE_Transmit(const uint16_t *array, uint32_t len, uint32_t len_max, uint32_t val_max, uint8_t colour) {
    len = len > len_max ? len_max : len;    // 长度太长就截断不画后面的了
    if (len < LINE_X_MAX) {     // 点数不足以填满x轴，循环len。从数组转换至坐标
        double x_k =(double)LINE_X_MAX / len_max;
        double y_k = (double)LINE_Y_MAX / val_max;
        for (uint32_t i = 0; i < len - 1; ++i) {
            auto x = (uint32_t)std::round(i * x_k);
            auto y = (uint32_t)std::round(array[i] * y_k);
            auto x_next = (uint32_t)std::round((i + 1) * x_k);
            auto y_next = (uint32_t)std::round(array[i + 1] * y_k);
            HMI_LINE_Transmit(x, y, x_next, y_next, colour);
        }
    }
    else {  // LINE_X_MAX，循环LINE_X_MAX。x轴最近邻插值从坐标转回数组，y轴仍然从数组转换为坐标
        double x_k = len_max / (double)LINE_X_MAX;
        double y_k = (double)LINE_Y_MAX / val_max;
        auto loop_times = (uint32_t)std::floor(LINE_X_MAX * len / len_max) - 1;
        for (uint32_t i = 0; i < loop_times; ++i) {
            auto index = (uint32_t)std::round(i * x_k);     // 最近邻还原
            auto y = (uint32_t)std::round(array[index] * y_k);
            auto index_next = (uint32_t)std::round((i + 1) * x_k);
            auto y_next = (uint32_t)std::round(array[index_next] * y_k);
            HMI_LINE_Transmit(i, y, i + 1, y_next, colour);
        }
    }
}

// 通过切换至当前页面，从而达到刷新的目的
void HMI_Refresh(uint32_t page_id) {
    sprintf(msg, "page page%d" "\xff\xff\xff", (int)page_id);
    HMI_Transmit(msg);
}

void HMI_FSK_Transmit(float32_t rate, float32_t fsk_h) {
    char msg__[50] = {0};
    HMI_TXT_Transmit("DIGITAL MODULATION: FSK", 0);
    sprintf(msg__, "rate: %d", (int)std::round(rate));
    HMI_TXT_Transmit(msg__, 1);

    ftoa((double)fsk_h, float_data_, 3);
    sprintf(msg__, "h: %s", float_data_);
    HMI_TXT_Transmit(msg__, 2);
}
void HMI_ASK_Transmit(float32_t rate) {
    char msg__[50] = {0};
    HMI_TXT_Transmit("DIGITAL MODULATION: ASK", 0);
    sprintf(msg__, "rate: %d", (int)std::round(rate));
    HMI_TXT_Transmit(msg__, 1);
    HMI_TXT_Transmit(" ", 2);
}
void HMI_PSK_Transmit(float32_t rate) {
    char msg__[50] = {0};
    HMI_TXT_Transmit("DIGITAL MODULATION: PSK", 0);
    sprintf(msg__, "rate: %d", (int)std::round(rate));
    HMI_TXT_Transmit(msg__, 1);
    HMI_TXT_Transmit(" ", 2);
}
void HMI_AM_Transmit(float32_t freq_use, float32_t ma) {
    char msg__[50] = {0};
    HMI_TXT_Transmit("ANALOG MODULATION: AM", 0);

    ftoa((double)ma, float_data_, 3);
    sprintf(msg__, "MA: %s", float_data_);
    HMI_TXT_Transmit(msg__, 1);

    sprintf(msg__, "FREQ: %d", (int)std::round(freq_use));
    HMI_TXT_Transmit(msg__, 2);
}
void HMI_FM_Transmit(float32_t freq_use, float32_t mf) {
    char msg__[50] = {0};
    HMI_TXT_Transmit("ANALOG MODULATION: FM", 0);

    ftoa((double)mf, float_data_, 3);
    sprintf(msg__, "MF: %s    DEV: %d", float_data_, (int)std::round(freq_use * mf));
    HMI_TXT_Transmit(msg__, 1);

    sprintf(msg__, "FREQ: %d", (int)std::round(freq_use));
    HMI_TXT_Transmit(msg__, 2);
}
void HMI_CW_Transmit() {
    HMI_TXT_Transmit("CW", 0);
    HMI_TXT_Transmit("CW", 1);
    HMI_TXT_Transmit("CW", 2);
}

void ftoa(double f_num, char *msg, int point_num) {
    int int_num = (int)f_num;
    double point = (f_num - (double)int_num);
    if (int_num == 0) {
        msg[0] = '0';
        msg[1] = '.';
        int temp = (int)(point * (double)pow(10, point_num));
        itoa((int)(temp), msg + 2, 10);
    }
    else {
        int_num = (int_num + point / 10) * pow(10, point_num + 1);
        itoa(int_num, msg, 10);
        msg[strlen(msg) - point_num - 1] = '.';
    }

}
