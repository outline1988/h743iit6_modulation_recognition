/**
 * 2023/3/27
 * 鉴于AD9910的驱动写的过于垃圾，此驱动为重写版本（虽然重写版本也很垃圾）
 * by qyb
 */


#include "main.h"
#include "AD9910.h"
#include "math.h"
#include "SI5351.h"


#define __AD9910_PWR_SET 		( HAL_GPIO_WritePin(GPIOI, GPIO_PIN_10, GPIO_PIN_SET) )
#define __AD9910_PWR_CLR		( HAL_GPIO_WritePin(GPIOI, GPIO_PIN_10, GPIO_PIN_RESET) )

#define __AD9910_SDIO_SET		( HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET) )
#define __AD9910_SDIO_CLR		( HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET) )

#define __AD9910_DRH_SET		( HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET) )
#define __AD9910_DRH_CLR		( HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET) )

#define __AD9910_DRO_SET		( HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET) )
#define __AD9910_DRO_CLR		( HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET) )

#define __AD9910_IOUP_SET		( HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET) )
#define __AD9910_IOUP_CLR		( HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET) )

#define __AD9910_PF0_SET		( HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET) )
#define __AD9910_PF0_CLR		( HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET) )

#define __AD9910_PF1_SET		( HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET) )
#define __AD9910_PF1_CLR		( HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_RESET) )

#define __AD9910_PF2_SET		( HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET) )
#define __AD9910_PF2_CLR		( HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET) )

#define __AD9910_REST_SET		( HAL_GPIO_WritePin(GPIOI, GPIO_PIN_9, GPIO_PIN_SET) )
#define __AD9910_REST_CLR		( HAL_GPIO_WritePin(GPIOI, GPIO_PIN_9, GPIO_PIN_RESET) )

#define __AD9910_SCLK_SET		( HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET) )
#define __AD9910_SCLK_CLR		( HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET) )

#define __AD9910_DRC_SET		( HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET) )
#define __AD9910_DRC_CLR		( HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET) )

#define __AD9910_OSK_SET		( HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET) )
#define __AD9910_OSK_CLR		( HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET) )

#define __AD9910_CS_SET			( HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET) )
#define __AD9910_CS_CLR			( HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET) )

// CFR1 CFR2 CFR3控制寄存器, 数据格式MSB, 写入寄存器以下标0开始，故下标0为最高位byte
//uint8_t cfr1[] = {0x00, 0x40, 0x00, 0x00};       // cfr1, inverse sinc filter enable[22]
//
//uint8_t cfr2[] = {0x01, 0x00, 0x00, 0x00};       // cfr2, enable amplitude scale from single tone profiles[24]
//
//uint8_t cfr3[] = {0x05, 0x0F, 0x41, 0x32};       // cfr3, 40M输入, 25倍频, ICP = 001 (小电流)
/* cfr3 config:
 * 0x05 = 0000 0101b, VC0=101
 * 0x0f = 0000 1111b, ICP=001 (小电流)
 * 0x41 = 0100 0001b, REFCLK和PLL使能，最终使用PLL的输出为系统时钟，所以REFCLK无用
 * 0x32 = 0011 0010b, 最后一位无效位，0011 001b是25倍频
 */

// MSB
//uint8_t stone_profile[8] = {0x3f, 0xff, 0x00, 0x00, 0x25, 0x09, 0x7b, 0x42};
/*
 * index_01[13 : 0]: amplitude scale factor(14 bit), amp: 0x0011_1111, 0x1111_1111;
 * index_23[15 : 0]: phase offset word(16 bit), phase: 0x0000_0000, 0x0010_0101;
 * index_4567[31 : 0]: frequency tuning word(32_bit), freq: 0x0000_1001, 0x0111_1011, 0x0100_0010.
 */


/*
 * index_0[15 : 0]: open
 * index_12[15 : 0]: address step rate
 * index_34[15 : 6]: waveform end address
 * idnex_56[15 : 6]: waveform start address
 * index_7[2 : 0]: RAM Profile 模式控制 data sheet P33
 */

/**
 * 	GPIO口连接 H743IIT6
 *   PWR				PI10
 *   SDIO				PC13
 *   DRH(DPH)			PB7
 *   DRO				PB9
 *   UP_DAT(IOUP)		PE5
 *   PF0				PE3
 *   PF1				PG13
 *   PF2				PF10
 *	 MAS_REST(REST)     PI9
 *	 SCLK				PB8
 *	 DRCTL(DRC)			PE6
 *	 OSK				PE4
 *	 CS					PB4
 */

/*
 * GPIO口初始化，所有GPIO皆配置为开漏，上拉模式
 * 若在MX配置了这些GPIO，可不用调用该函数
 */
void AD9910_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

	GPIO_Initure.Pin = GPIO_PIN_4 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_PULLUP;
	GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);


	GPIO_Initure.Pin = GPIO_PIN_13;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_PULLUP;
	GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOC, &GPIO_Initure);

    GPIO_Initure.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOE, &GPIO_Initure);

    GPIO_Initure.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOI, &GPIO_Initure);

    GPIO_Initure.Pin = GPIO_PIN_10;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_Initure);

    GPIO_Initure.Pin = GPIO_PIN_13;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOG, &GPIO_Initure);


}


//=====================================================================


/*
 * 向AD9910发送1byte的数据，高位先发
 */
static void AD9910_Write_Byte(uint8_t byte_data) {
	for (int i = 0; i < 8; i++) {
		__AD9910_SCLK_CLR;	// 时钟置低

		(byte_data & 0x80) == 0 ? __AD9910_SDIO_CLR : __AD9910_SDIO_SET;
		byte_data <<= 1;

		__AD9910_SCLK_SET;	// 时钟升高，同时上升沿发送数据
   }
	__AD9910_SCLK_CLR;	// 默认状态时钟为低
 }




/*
 * 实际频率freq转化为频率调谐字 (16 bit)
 * 向profilex写入频率调谐字 (单频模式)，需要先开启单频模式
 * freq_tuning_word (FTW) 的范围为[0 : 2147483647 (2^31 - 1)]
 *
 * f_out = (FTW / 2^32) * f_sys ==> FTW = (2^32 * f_out) / f_sys = (2^32 / f_sys) * f_out = 4.294967296
 * 实测中，频率有误差，所以需要打表补偿
 */

/*
 * 返回单频模式下对应于amp的AFS
 */
static uint16_t AD9910_Stone_Gen_Profile_Ampconvert(double amp) {
	return (uint16_t)amp;
}

static uint32_t AD9910_STone_FTW_Compensate(double freq) {
	uint32_t FTW_compensate = 0;
	if (freq > 19e3) {
		FTW_compensate = 5;
	}
	else if (freq > 17.5e3) {
		FTW_compensate = 4;
	}
	else if (freq > 12.5e3) {
		FTW_compensate = 3;
	}
	else if (freq > 7.5e3) {
		FTW_compensate = 2;
	}
	else if (freq > 4e3) {
		FTW_compensate = 1;
	}
	else {
		FTW_compensate = 0;
	}
	return FTW_compensate;
}

/*
 * 返回对应于freq的FTW
 */
static uint32_t AD9910_Stone_Gen_Profile_Freqconvert(double freq) {
//	uint32_t FTW_compensate = 5;		// 20khz
//	uint32_t FTW_compensate = 4;		// 18khz
//	uint32_t FTW_compensate = 3;		// 15khz
//	uint32_t FTW_compensate = 2;		// 10khz
//	uint32_t FTW_compensate = 1;		// 05khz
	uint32_t FTW_compensate = AD9910_STone_FTW_Compensate(freq);
	return (uint32_t)round((uint32_t)freq * 4.294967296)
									+ FTW_compensate;  // 4.294967296 = (2^32) / 1e9
}
/*
 * 返回对应于phase的POW
 */
static uint16_t AD9910_Stone_Gen_Profile_Phaseconvert(double phase) {
	return (uint16_t)round(phase * 9.587379924285257e-5);	//			9.587379924285257e-5 = phase * (2 * PI) / 65536;
}



/*
 * 向stone_profile写入振幅比例因子 (14bit)
 * amp_scale_factor范围为 [0 : 2^14 - 1(16383)]
 * profile是个含8个uint8_t类型的数组
 */
static void AD9910_Stone_Gen_Profile_ASF(uint8_t *stone_profile, uint16_t amp_scale_factor) {
	stone_profile[0]=(uint8_t)(amp_scale_factor >> 8);
	stone_profile[1]=(uint8_t)amp_scale_factor;
}

/*
 * 向stone_profile写入FTW(32 bit)
 * profile是个含8个uint8_t类型的数组
 */
static void AD9910_Stone_Gen_Profile_FTW(uint8_t *stone_profile, uint32_t freq_tuning_word) {
	stone_profile[4]=(uint8_t)(freq_tuning_word >> 24);
	stone_profile[5]=(uint8_t)(freq_tuning_word >> 16);
	stone_profile[6]=(uint8_t)(freq_tuning_word >> 8);
	stone_profile[7]=(uint8_t)freq_tuning_word;
}

/*
 * 向stone_profile写入相位相位偏移字POW(16 bit)
 * profile是个含8个uint8_t类型的数组
 */
static void AD9910_Stone_Gen_Profile_POW(uint8_t *stone_profile,
										 uint16_t phase_offset_word) {
	stone_profile[2] = (uint8_t)(phase_offset_word >> 8);
	stone_profile[3] = (uint8_t)phase_offset_word;
}



/*
 * 生成单频模式的profile寄存器的值，包含AFS(14 bit)、FTW(32 bit)和POW(16 bit)
 * profile是个含8个uint8_t类型的数组
 * 有0->7一次代表着profile寄存器的高位到低位
 */
static void AD9910_STone_Gen_Profile_WORD(uint8_t *stone_profile, uint16_t amp_factor_scale,
													   uint32_t freq_tuning_word,
													   uint16_t phase_offset_word) {
	// 根据参数设置profile的值
	AD9910_Stone_Gen_Profile_ASF(stone_profile, amp_factor_scale);
	AD9910_Stone_Gen_Profile_FTW(stone_profile, freq_tuning_word);
	AD9910_Stone_Gen_Profile_POW(stone_profile, phase_offset_word);

}


/*
 * 生成单频模式下传入参数的profile数据
 * stone_profile为要设置的profile数据
 * 将amp、freq和phase转化为word后再生成profile数据
 */
static void AD9910_Stone_Gen_Profile(uint8_t *stone_profile, double amp, double freq, double phase) {
	uint16_t amp_factor_scale = AD9910_Stone_Gen_Profile_Ampconvert(amp);
	uint32_t freq_tuning_word = AD9910_Stone_Gen_Profile_Freqconvert(freq);
	uint16_t phase_offset_word = AD9910_Stone_Gen_Profile_Phaseconvert(phase);
	AD9910_STone_Gen_Profile_WORD(stone_profile, amp_factor_scale, freq_tuning_word, phase_offset_word);
}

/*
 * 默认单频模式下的cfr寄存器配置
 */
static void AD9910_Defaut_CFR(uint32_t *cfr) {
	// CFR1 CFR2 CFR3控制寄存器, 数据格式MSB, 写入寄存器以下标0开始，故下标0为最高位byte
	cfr[0] = 0x00402000;       // cfr1, inverse sinc filter enable[22]

	cfr[1] = 0x01000000;       // cfr2, enable amplitude scale from single tone profiles[24]

	cfr[2] = 0x050f4132;       // cfr3, 40M输入, 25倍频, ICP = 001 (小电流)
	/* cfr3 config:
	 * 0x05 = 0000 0101b, VC0=101
	 * 0x0f = 0000 1111b, ICP=001 (小电流)
	 * 0x41 = 0100 0001b, REFCLK和PLL使能，最终使用PLL的输出为系统时钟，所以REFCLK无用
	 * 0x32 = 0011 0010b, 最后一位无效位，0011 001b是25倍频
	 */
}
/*
 * 生成单频模式下的cfr寄存器数据
 * 在默认配置的基础上修改
 * cfr时=是包含三个uint32_t即cfr1 cfr2 cfr3数据的数组
 */
static void AD9910_Stone_Gen_CFR(uint32_t *cfr) {
	AD9910_Defaut_CFR(cfr);
}

/*
 *	使用函数形式参数变量cfr(包含三个uint32_t的元素，每个uint32_t代表一个cfr寄存器)来配置AD9910的cfr寄存器
 */
static void AD9910_Write_CFR(uint32_t *cfr) {
	uint8_t cfr_address = 0x00;
	for (int i = 0; i < 3; ++i, ++cfr_address) {
		__AD9910_CS_CLR;		// 片选，低电平有效（选择）
		AD9910_Write_Byte(cfr_address);
		for (int m = 3; m != -1; --m) {	// m的范围[0, 3]
			AD9910_Write_Byte( (uint8_t)(cfr[i] >> (m * 8)) );
		}
		__AD9910_CS_SET;
		for (int k = 0; k < 10; ++k) {	// 延迟循环
			;
		}
	}

	// 更新I/O UP
	__AD9910_IOUP_CLR;
	for(int k =0 ; k < 10; ++k) {
		;
	}
	__AD9910_IOUP_SET;
	HAL_Delay(1);
}

/**
 * 向AD9910的PROFILEx发送形参profile配置
 * profile是个含8个uint8_t类型的数组
 */
static void AD9910_Write_Profile(uint8_t *profile,
						AD9910_ProfileType AD9910_PROFILE_x) {
	uint8_t m, k;

	uint8_t profile_x_address = 0x0e + (uint8_t)AD9910_PROFILE_x;


	__AD9910_CS_CLR;
	AD9910_Write_Byte(profile_x_address);    //发送profile0控制字地址
	for (m = 0; m < 8; m++) {
		AD9910_Write_Byte(profile[m]);
	}
	__AD9910_CS_SET;
	for(k = 0; k < 10; ++k);


	__AD9910_IOUP_CLR;	// I/O_UPDATE刷新
 	for(k = 0; k < 10; ++k);
 	__AD9910_IOUP_SET;
 	HAL_Delay(1);
}

static void AD9910_Select_Profile(AD9910_ProfileType AD9910_PROFILE_x) {
	// 选择PROFILEx
	(AD9910_PROFILE_x & 0b001) == 0b001 ? __AD9910_PF0_SET : __AD9910_PF0_CLR;
	(AD9910_PROFILE_x & 0b010) == 0b010 ? __AD9910_PF1_SET : __AD9910_PF1_CLR;
	(AD9910_PROFILE_x & 0b100) == 0b100 ? __AD9910_PF2_SET : __AD9910_PF2_CLR;
}


/*
 * 选择AD9910的模式
 */
void AD9910_RAM_Gen_CFR(uint32_t *cfr);
void AD9910_DRG_Gen_CFR(uint32_t *cfr);
static void AD9910_Select_Mode(AD9910_ModeType AD9910_Mode) {
	uint32_t cfr[3];
	switch(AD9910_Mode) {
		case AD9910_STone_Mode:
			AD9910_Stone_Gen_CFR(cfr);
			break;
		case AD9910_RAM_Mode:
			AD9910_RAM_Gen_CFR(cfr);
			break;
		case AD9910_DRG_Mode:
			AD9910_DRG_Gen_CFR(cfr);
			break;
	}
	AD9910_Write_CFR(cfr);
}

/*
 * 初始化AD9910
 */
void AD9910_Init(void) {
    AD9910_GPIO_Init();

	__AD9910_PWR_CLR;		// 外部省电模式，不使用则接地

	// 初始化，默认选择了Profile0，Profilex中的数据代表着某个模式（单频模式）的配置信息（振幅、相位和频率）
	AD9910_Select_Profile(AD9910_Profile_0);

	__AD9910_DRC_CLR;		// 数字斜坡模式相关，不使用则接地
	__AD9910_DRH_CLR;		// 同上

	// 清空所有配置为默认
	__AD9910_REST_SET;	// 主机复位，所有寄存器和memory设置为默认值，高电平有效
	HAL_Delay(5);
	__AD9910_REST_CLR;

	// 向CFR寄存器写入默认配置 (打开单频模式)
	uint8_t profile_default[8];

	AD9910_Stone_Gen_Profile(profile_default, 16383, 20000, 0);	// 最大幅度，20khz的正弦波

	AD9910_Write_Profile(profile_default, AD9910_Profile_0);	// 由PF0 PF1 PF2引脚选择的Profile_0
	AD9910_Select_Mode(AD9910_STone_Mode);
}

/*
 * 从其他模式切换至单频模式时使用
 */
void AD9910_STone_Switch(double amp, double freq, double phase) {
	uint8_t profile[8];

	AD9910_Stone_Gen_Profile(profile, amp, freq, phase);	// 生成profile和cfr配置
	AD9910_Write_Profile(profile, AD9910_Profile_0);	// 写入配置

	AD9910_Select_Mode(AD9910_STone_Mode);

	AD9910_Select_Profile(AD9910_Profile_0);

}

/*
 * 当前是单频模式，改变正弦波的参数
 */

void AD9910_STone_Set(double amp, double freq, double phase) {
	uint8_t profile[8];

	AD9910_Stone_Gen_Profile(profile, amp, freq, phase);	// 生成profile和cfr配置

	AD9910_Write_Profile(profile, AD9910_Profile_0);	// 写入配置

	AD9910_Select_Profile(AD9910_Profile_0);

}

/*
 * address_step_rate: 就是计数器的模值
 * waveform_end_address: RAM数据结束的地址
 * waveform_start_address: RAM数据开始的地址
 * [waveform_start_address, waveform_end_address]即为RAM数据的范围
 */
static void AD9910_RAM_Gen_Profile(uint8_t *profile,
							uint16_t waveform_start_address,
							uint16_t waveform_end_address,
							uint16_t address_step_rate) {
	profile[0] = 0x00;

	profile[1] = (uint8_t)(address_step_rate >> 8);
	profile[2] = (uint8_t)address_step_rate;

	profile[3] = (uint8_t)(waveform_end_address >> 2);
	profile[4] = (uint8_t)(waveform_end_address << 6);

	profile[5] = (uint8_t)(waveform_start_address >> 2);
	profile[6] = (uint8_t)(waveform_start_address << 6);

	profile[7] = 0x04;
}

/*
 * 生成RAM模式的CFR寄存器配置
 */
void AD9910_RAM_Gen_CFR(uint32_t *cfr) {
	AD9910_Defaut_CFR(cfr);

	cfr[0] |= 0xe0000000; 	// RAM使能，极性控制控制(RAM数据以控制幅度为主)，覆盖单频模式
	// 极性控制的最大峰峰值为振幅相同，相位相差180度
}


/*
 * 生成脉冲波表数据，幅度控制(cfr1[30 : 29], table 12 in data sheet)
 * 共num_samples，小于等于1024
 * 为了使占空比尽量准确，应让num_samples尽可能大
 */
static void AD9910_RAM_Gen_RAMData_Pulse(uint32_t *ram_waveform_data, uint16_t num_samples,
															   uint16_t amp_factor_scale,
															   double duty) {
	uint16_t phase_0 = 0x0000;		// 相位0度
	uint16_t phase_180 = 0x7fff;	// 相位180度
	for (uint16_t i = 0; i != num_samples; ++i) {
		ram_waveform_data[i] = i < num_samples * duty ?
							((uint32_t)(amp_factor_scale) << 2) + ((uint32_t)phase_0 << 16) :
							((uint32_t)(amp_factor_scale) << 2) + ((uint32_t)phase_180 << 16);
	}
}

/*
 * 向RAM中写入数据
 */
static void AD9910_RAM_Write_RAMData(uint32_t *ram_waveform_data, uint16_t num_samples) {
	uint8_t k;

	__AD9910_CS_CLR;

	AD9910_Write_Byte(0x16);    // 发送ram控制字地址
	for (uint16_t i = 0; i < 1024; i++) {
		if (i < num_samples) {
			AD9910_Write_Byte((uint8_t)(ram_waveform_data[i] >> 24));
			AD9910_Write_Byte((uint8_t)(ram_waveform_data[i] >> 16));
			AD9910_Write_Byte((uint8_t)(ram_waveform_data[i] >> 8));
			AD9910_Write_Byte((uint8_t)ram_waveform_data[i]);
		}
		else {	// 写入剩余的RAM数据
			AD9910_Write_Byte(0x00);
			AD9910_Write_Byte(0x00);
			AD9910_Write_Byte(0x00);
			AD9910_Write_Byte(0x00);
		}

	}
	__AD9910_CS_SET;

	for(k = 0; k < 10; ++k) {
		;
	}

	__AD9910_IOUP_CLR;
	for(k = 0; k < 10; ++k) {
		;
	}
	__AD9910_IOUP_SET;

	HAL_Delay(1);
}

/*
 * 返回RAM模式下对应于amp的AFS
 */
static uint16_t AD9910_RAM_Gen_Profile_Ampconvert(double amp) {
	return (uint16_t)amp;
}

/*
 * 通过频率得到合适的num_samples和address_step_rate
 * factor1对应num_samples
 * factor2对应address_step_rate
 */
static void divide_num(double multip, uint16_t *factor1, uint16_t *factor2) {
	uint16_t factor1_start = 500;	// 对应num_samples
	uint16_t factor1_end = 1025;
	uint16_t temp_factor2;
    int error;
    int error_min = 100;
    for (int temp_factor1 = factor1_start; temp_factor1 < factor1_end; ++temp_factor1) {
        temp_factor2 = multip / temp_factor1;
        error = fabs(temp_factor1 * temp_factor2 - multip);
        if (error < error_min) {
            error_min = error;
            *factor1 = temp_factor1;
            *factor2 = temp_factor2;
        }
    }
}
static void AD9910_RAM_Gen_Profile_Freqconvert(double freq, uint16_t *num_samples, uint16_t *address_step_rate) {
	divide_num(250e6 / freq, num_samples, address_step_rate);
}
/*
 * 切换至RAM模式生成方波
 */
static void AD9910_RAM_Switch_Pulse_WORD(uint16_t amp_factor_scale,	// 振幅AFS
								  uint16_t num_samples, uint16_t address_step_rate,	// 关于频率的量
								  double duty) {
	static uint32_t ram_waveform_data[1024];
	uint8_t profile[8];

	AD9910_RAM_Gen_RAMData_Pulse(ram_waveform_data, num_samples, amp_factor_scale, duty);
	AD9910_RAM_Gen_Profile(profile, 0, num_samples - 1, address_step_rate);

	AD9910_Write_Profile(profile, AD9910_Profile_0);
	AD9910_RAM_Write_RAMData(ram_waveform_data, num_samples);

	AD9910_Select_Mode(AD9910_RAM_Mode);


	AD9910_Select_Profile(AD9910_Profile_0);


}

void AD9910_RAM_Switch_Pulse(double amp, double freq, double duty) {
	uint16_t num_samples, address_step_rate;
	uint16_t amp_factor_scale = AD9910_RAM_Gen_Profile_Ampconvert(amp);
	AD9910_RAM_Gen_Profile_Freqconvert(freq, &num_samples, &address_step_rate);
    // SI5351 must init before
    double si5351_freq = num_samples * address_step_rate * freq * 4 / 25;
    SI5351_SetCLK2(si5351_freq);
	AD9910_RAM_Switch_Pulse_WORD(amp_factor_scale, num_samples, address_step_rate, duty);
}

/*
 * RAM模式下正弦波，用于解决单频模式下小于5khz时无法达到10e-4的精度的问题
 */
static void AD9910_RAM_Gen_RAMData_Sine(uint32_t *ram_waveform_data, uint16_t num_samples, uint16_t amp_factor_scale) {
	uint32_t phase_360 = 0x10000;	// 相位360度（闭区间）
	double delta_phase = (double)phase_360 / (double)num_samples;
	for (uint16_t i = 0; i != num_samples; ++i) {
		ram_waveform_data[i] = ((uint32_t)(amp_factor_scale) << 2) + ((uint32_t)round(delta_phase * i) << 16);
	}
}

static void AD9910_RAM_Switch_Sine_WORD(uint16_t amp_factor_scale,	// 振幅AFS
								  int16_t num_samples,
								  int16_t address_step_rate) {	// 关于频率的量
	static uint32_t ram_waveform_data[1024];
	uint8_t profile[8];

	AD9910_RAM_Gen_RAMData_Sine(ram_waveform_data, num_samples, amp_factor_scale);
	AD9910_RAM_Gen_Profile(profile, 0, num_samples - 1, address_step_rate);

	AD9910_Write_Profile(profile, AD9910_Profile_0);
	AD9910_RAM_Write_RAMData(ram_waveform_data, num_samples);

	AD9910_Select_Mode(AD9910_RAM_Mode);


	AD9910_Select_Profile(AD9910_Profile_0);

}

void AD9910_RAM_Switch_Sine(double amp, double freq) {
	uint16_t num_samples, address_step_rate;
	uint16_t amp_factor_scale = AD9910_RAM_Gen_Profile_Ampconvert(amp);
	AD9910_RAM_Gen_Profile_Freqconvert(freq, &num_samples, &address_step_rate);

	AD9910_RAM_Switch_Sine_WORD(amp_factor_scale, num_samples, address_step_rate);
}

void AD9910_Switch_Sine(double amp, double freq) {
	if (freq > 5500) {	// 单频模式
		AD9910_STone_Switch(amp, freq, 0);
	}
	else {
		AD9910_RAM_Switch_Sine(amp, freq);
	}
}


/*
 * DRG模式
 */
/*
 * 返回对应于freq的FTW
 */
static uint32_t AD9910_DRG_Freqconvert(double freq) {
	uint32_t freq_compensate = 0;
	return (uint32_t)round((uint32_t)freq * 4.294967296)
									+ freq_compensate;  // 4.294967296 = (2^32) / 1e9
}

/*
 * DRG限值寄存器，位置在0x0b
 */
uint64_t AD9910_DRG_Gen_Limit(uint32_t upper_limit,
						  	  uint32_t lower_limit) {
	return ((uint64_t)upper_limit << 32)
										+ ((uint64_t)lower_limit);
}

/*
 * DRG步进寄存器，位置在0x0c
 */
uint64_t AD9910_DRG_Gen_Step(uint32_t decrement_step_size,
						 	 uint32_t increment_step_size) {
	return ((uint64_t)decrement_step_size << 32)
										+ ((uint64_t)increment_step_size);
}

/*
 * DRG步进速率寄存器，位置在0x0d
 */
uint32_t AD9910_DRG_Gen_Rate(uint16_t negative_slop_rate,
						 	 uint16_t positive_slop_rate) {
	return ((uint32_t)negative_slop_rate << 16)
										+ (uint32_t)positive_slop_rate;
}

void AD9910_DRG_Write_Limit(uint64_t digital_ramp_limit_register) {
	uint8_t digital_ramp_limit_register_address = 0x0b;

	__AD9910_CS_CLR;
	AD9910_Write_Byte(digital_ramp_limit_register_address);    //发送profile0控制字地址
	for (int i = 7; i != -1; --i) {		// [7 : 0]
		AD9910_Write_Byte( (uint8_t)(digital_ramp_limit_register >> (i * 8)) );
	}
	__AD9910_CS_SET;
	for(int k = 0; k < 10; ++k);


	__AD9910_IOUP_CLR;	// I/O_UPDATE刷新
 	for(int k = 0; k < 10; ++k);
 	__AD9910_IOUP_SET;
 	HAL_Delay(1);
}

void AD9910_DRG_Write_Step(uint64_t digital_ramp_step_size_register) {
	uint8_t digital_ramp_step_size_register_address = 0x0c;

	__AD9910_CS_CLR;
	AD9910_Write_Byte(digital_ramp_step_size_register_address);    //发送profile0控制字地址
	for (int i = 7; i != -1; --i) {		// [7 : 0]
		AD9910_Write_Byte( (uint8_t)(digital_ramp_step_size_register >> (i * 8)) );
	}
	__AD9910_CS_SET;
	for(int k = 0; k < 10; ++k);


	__AD9910_IOUP_CLR;	// I/O_UPDATE刷新
 	for(int k = 0; k < 10; ++k);
 	__AD9910_IOUP_SET;
 	HAL_Delay(1);
}

void AD9910_DRG_Write_Rate(uint32_t digital_ramp_rate_register) {
	uint8_t digital_ramp_rate_register_address = 0x0d;

	__AD9910_CS_CLR;
	AD9910_Write_Byte(digital_ramp_rate_register_address);    //发送profile0控制字地址
	for (int i = 3; i != -1; --i) {		// [7 : 0]
		AD9910_Write_Byte( (uint8_t)(digital_ramp_rate_register >> (i * 8)) );
	}
	__AD9910_CS_SET;
	for(int k = 0; k < 10; ++k);


	__AD9910_IOUP_CLR;	// I/O_UPDATE刷新
 	for(int k = 0; k < 10; ++k);
 	__AD9910_IOUP_SET;
 	HAL_Delay(1);
}

void AD9910_DRG_Write_Register(uint64_t digital_ramp_limit_register,
							   uint64_t digital_ramp_step_size_register,
							   uint32_t digital_ramp_rate_register) {
	AD9910_DRG_Write_Limit(digital_ramp_limit_register);
	AD9910_DRG_Write_Step(digital_ramp_step_size_register);
	AD9910_DRG_Write_Rate(digital_ramp_rate_register);
}

void AD9910_DRG_Gen_CFR(uint32_t *cfr) {
	AD9910_Defaut_CFR(cfr);

//	cfr[0] |= 0x00001000;
	cfr[0] |= 0x00004000;	// 自动清除相位累加（不知道为什么直接清除不行，要自动清除才行）
	cfr[1] |= 0x00080000; 	// DRG使能
}

/*
 * 将上限频率和下限频率
 * 需要满足 freq_upper > freq_lower
 * 不知道为什么，向下扫频实现不了
 * 不知道为什么，第一次扫频和第二次扫频中间若隔了个单频模式，第二次扫频实现不了，得先使用AD9910_Init()初始化
 */
void AD9910_DRG_Switch_SweepFreq(double freq_lower, double freq_upper, double sweep_time,
								 	 	 	 AD9910_DRG_UpDownType AD9910_DRG_UpDown) {

	uint32_t freq_tuning_word_upper = AD9910_DRG_Freqconvert(freq_upper);
	uint32_t freq_tuning_word_lower = AD9910_DRG_Freqconvert(freq_lower);


	uint16_t step_rate = 0xffff;	// 默认步进计数值为最大0xffff，表示步进得很快

	uint32_t step_size = round( (freq_tuning_word_upper - freq_tuning_word_lower) * step_rate / (sweep_time * 250e6) );


	uint64_t digital_ramp_limit_register = AD9910_DRG_Gen_Limit(freq_tuning_word_upper, freq_tuning_word_lower);
	uint64_t digital_ramp_step_size_register = AD9910_DRG_Gen_Step(step_size, step_size);
	uint32_t digital_ramp_rate = AD9910_DRG_Gen_Rate(step_rate, step_rate);


	AD9910_DRG_Write_Register(digital_ramp_limit_register, digital_ramp_step_size_register, digital_ramp_rate);

	__AD9910_DRH_CLR;
	(AD9910_DRG_UpDown == AD9910_DRG_Negative) ? __AD9910_DRC_CLR : __AD9910_DRC_SET;	// up or down by Pin DRG

	AD9910_Select_Mode(AD9910_DRG_Mode);

	HAL_Delay(sweep_time * 5000);

}

void AD9910_Reset() {
    __AD9910_REST_SET;	// 主机复位，所有寄存器和memory设置为默认值，高电平有效
    HAL_Delay(5);
    __AD9910_REST_CLR;
}






