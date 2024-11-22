/**
 * 2023/3/27
 * 鉴于AD9910的驱动写的过于垃圾，此驱动为重写版本
 * by qyb
 */
#ifndef _AD9910_H
#define _AD9910_H

#include "main.h"

/**
 * 	GPIO口连接 H743IIT6
 *   PWR				PI10 1
 *   SDIO				PC13
 *   DRH(DPH)			PB7
 *   DRO				PB9
 *   UP_DAT(IOUP)		PE5
 *   PF0				PE3
 *   PF1				PG13 1
 *   PF2				PF10
 *	 MAS_REST(REST)     PI9 1
 *	 SCLK				PB8
 *	 DRCTL(DRC)			PE6
 *	 OSK				PE4
 *	 CS					PB4
 *
 */



typedef enum {
	AD9910_Profile_0 = 0,
	AD9910_Profile_1,
	AD9910_Profile_2,
	AD9910_Profile_3,
	AD9910_Profile_4,
	AD9910_Profile_5,
	AD9910_Profile_6,
	AD9910_Profile_7
} AD9910_ProfileType;

typedef enum {
	AD9910_STone_Mode = 0,
	AD9910_RAM_Mode,
	AD9910_DRG_Mode
} AD9910_ModeType;

typedef enum {
	AD9910_DRG_Positive = 0,
	AD9910_DRG_Negative
} AD9910_DRG_UpDownType;
 
void AD9910_GPIO_Init(void);
void AD9910_Init(void);

void AD9910_STone_Switch(double amp, double freq, double phase);
void AD9910_STone_Set(double amp, double freq, double phase);

void AD9910_RAM_Switch_Pulse(double amp, double freq, double duty);
void AD9910_RAM_Switch_Sine(double amp, double freq);

void AD9910_Switch_Sine(double amp, double freq);

void AD9910_DRG_Switch_SweepFreq(double freq_lower, double freq_upper, double sweep_time,
								 	 	 	 AD9910_DRG_UpDownType AD9910_DRG_UpDown);

void AD9910_Reset();

#endif




