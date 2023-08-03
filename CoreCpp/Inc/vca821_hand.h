//
// Created by outline on 2023/7/6.
//

#ifndef _VCA821_HAND_H
#define _VCA821_HAND_H

#include "main_cpp.h"
#include "dac_output.h"

class Vca821_hand {
public:
    Vca821_hand(): current_dac_value(0), current_vol_value(0), current_gain_value(0),
                    hdac(nullptr), channel(0) {}
    explicit Vca821_hand(DAC_HandleTypeDef *__hdac, uint32_t __channel): current_dac_value(0), current_vol_value(0),
                current_gain_value(0), hdac(__hdac), channel(__channel) { DAC_OUTPUT_Init(hdac); }

    [[nodiscard]] uint32_t current_dacvalue() const { return current_dac_value; }
    [[nodiscard]] double current_volvalue() const { return current_vol_value; }
    [[nodiscard]] double current_gainvalue() const { return current_gain_value; }

    void set_dacvalue(uint32_t dacvalue);
    void set_volvalue(double vol);
    void set_gainvalue(double gain);

    double gain_error(double vpp_current, double vpp_reference) const;
    double gain_error(double v_rms, double curent_rms, double power_reference) const;

    void uppdate_gain_error(double error, double kp = 1);
    void update_gain(double vpp_current, double vpp_reference, double kp = 1);

//    void update_gain(double v_rms, double curent_rms, double power_reference, double kp = 1);

private:
    uint32_t current_dac_value;
    double current_vol_value;
    double current_gain_value;
    DAC_HandleTypeDef *hdac;
    uint32_t channel;

    static uint32_t vol2dac(double volvalue) { return (uint32_t)(volvalue * 4096 / 3.3); }
    static double dac2vol(uint32_t dacvalue) { return (double)(dacvalue * 3.3 / 4096); }
    static double gain2vol(double gain);

};

#endif //_VCA821_HAND_H
