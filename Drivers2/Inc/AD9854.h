
#ifndef _AD9854_H_
#define _AD9854_H_

#include "main.h"


/*  With H743IIT6
 * PE6			RD
 * PE5			WR
 * PC13			UD	UDCLK
 * PB9			A1	SDO
 * PB7			A2	IORST
 * PB8			A0	SDIO
 * PB5			RT	RESET
 * PB4			SP
 */


//----------------------------------------------------------------

//-----------------------------------------------------------------

void AD9854_Init(void);
void AD9854_SINSet(uint32_t freq, uint16_t amp);
void AD9854_FSKInit(void);
void AD9854_FSKSet(uint32_t Freq1,uint32_t Freq2);
void AD9854_BPSKInit(void);
void AD9854_BPSKSet(uint16_t Phase1, uint16_t Phase2);
void AD9854_InitOSK(void);
void AD9854_SetOSK(uint8_t RateShape);
void AD9854_AMInit(void);
void AD9854_AMSet(uint16_t Shape);
void AD9854_RFSKInit(void);
void AD9854_RFSKSet(uint32_t Freq_Low, uint32_t Freq_High, uint32_t Freq_Up_Down, uint32_t FreRate);
void Sweep_out (void);
void AD9854_ChirpInit(void);
void AD9854_ChirpSet(unsigned long Freq_Low,unsigned long Freq_Up_Down,unsigned long FreRate);
void am_test (uint32_t fund_fre, uint8_t modulation);
#endif
