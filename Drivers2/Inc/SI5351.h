#ifndef _SI5351_H_
#define _SI5351_H_
// example(new):
//    SI5153_Init(&hi2c3);
//    SI5153_SetCLK(500e3, 1e6);

// example(old):
//    si5351_Init(0);
//    si5351_SetupCLK0(500e3, SI5351_DRIVE_STRENGTH_4MA);
//    si5351_SetupCLK2(144000000, SI5351_DRIVE_STRENGTH_4MA);
//    si5351_EnableOutputs((1<<0) | (1<<2));

void SI5351_Init(I2C_HandleTypeDef *hi2c);

void SI5351_SetCLK02(double freq0, double freq2);

void SI5351_SetCLK0(double freq0);

void SI5351_SetCLK2(double freq2);

#endif
