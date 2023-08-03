//
// Created by outline on 2023/7/6.
//

#include "main_cpp.h"
#include "vca821_hand.h"

void Vca821_hand::set_dacvalue(uint32_t dacvalue) {
    if (dacvalue > 4095) {
        dacvalue = 4095;
    }

    DAC_OUTPUT_SetDC(channel, dacvalue);
    current_dac_value = dacvalue;
}

void Vca821_hand::set_volvalue(double vol) {
    if (vol > 2) {
        vol = 2;
    }
    if (vol < 0) {
        vol = 0;
    }
    uint32_t dacvalue = vol2dac(vol);
    set_dacvalue(dacvalue);
    current_vol_value = vol;
}

/**
 * curve fitting
 * @param gain range from 1 to 50
 * @return the vol you should set to get the gain
 */
double Vca821_hand::gain2vol(double gain) {
    if (gain < 0) {
        return 0;
    }
    else if (gain < 9) {
        double a = 0.4516, b = 0.1841;
        return a * std::pow(gain, b);
    }
    else if (gain >= 50) {
        return 1.5;
    }
    else if (gain >= 42) {
        double a = 0.5705, b = 0.0140, c = 4.7321e-16, d = 0.6730;
        return a * std::exp(b * gain) + c * std::exp(d * gain);
    }
    else {
        double p1 = 0.0101, p2 = 0.5893;
        return p1 * gain + p2;
    }

}

void Vca821_hand::set_gainvalue(double gain) {
    if (gain < 0) {
        gain = 0;
    }
    if (gain > 50) {
        gain = 50;
    }
    double vol = gain2vol(gain);
    set_volvalue(vol);
    current_gain_value = gain;
}

/**
 * To give the error of pid, cal error of gain by current vpp and referent vpp.
 * @param vpp_current
 * @param vpp_reference
 * @return
 */
double Vca821_hand::gain_error(double vpp_current, double vpp_reference) const {
    return current_gain_value * (vpp_reference / vpp_current - 1);
}

double Vca821_hand::gain_error(double v_rms, double curent_rms, double power_reference) const {
    double power_current = v_rms * curent_rms;
    if (power_current > power_reference) {
        return -1 * current_gain_value * std::sqrt(1 - power_reference / power_current);
    }
    else {
        return current_gain_value * std::sqrt(power_reference / power_current - 1);
    }

}

void Vca821_hand::uppdate_gain_error(double error, double kp) {
    double gain = current_gain_value + kp * error;
    if (gain > 50) {
        gain = 50;
    }
    if (gain < 0) {
        gain = 0;
    }
    set_gainvalue(gain);
}

void Vca821_hand::update_gain(double vpp_current, double vpp_reference, double kp) {
    double error = gain_error(vpp_current, vpp_reference);
    uppdate_gain_error(error, kp);
}

//void Vca821_hand::update_gain(double v_rms, double curent_rms, double power_reference, double kp) {
//    double error = gain_error(v_rms, curent_rms, power_reference);
//    uppdate_gain_error(error, kp);
//}
