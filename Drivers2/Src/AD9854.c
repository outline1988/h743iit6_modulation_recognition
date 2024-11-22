#include "AD9854.h"
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

#define __AD9854_RD_SET 		( HAL_GPIO_WritePin ( GPIOE, GPIO_PIN_6, GPIO_PIN_SET   ) )
#define __AD9854_RD_CLR 		( HAL_GPIO_WritePin ( GPIOE, GPIO_PIN_6, GPIO_PIN_RESET ) )

#define __AD9854_WR_SET 		( HAL_GPIO_WritePin ( GPIOE, GPIO_PIN_5, GPIO_PIN_SET   ) )
#define __AD9854_WR_CLR 		( HAL_GPIO_WritePin ( GPIOE, GPIO_PIN_5, GPIO_PIN_RESET ) )

#define __AD9854_UD_SET  ( HAL_GPIO_WritePin ( GPIOC, GPIO_PIN_13, GPIO_PIN_SET   ) )
#define __AD9854_UD_CLR  ( HAL_GPIO_WritePin ( GPIOC, GPIO_PIN_13, GPIO_PIN_RESET ) )

#define __AD9854_RT_SET 	  ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_5, GPIO_PIN_SET   ) )
#define __AD9854_RT_CLR    ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_5, GPIO_PIN_RESET ) )

#define __AD9854_SP_SET     ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_4, GPIO_PIN_SET   ) )
#define __AD9854_SP_CLR     ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_4, GPIO_PIN_RESET ) )


#define __AD9854_SPI_IORST_SET ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_7, GPIO_PIN_SET   ) )
#define __AD9854_SPI_IORST_CLR ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_7, GPIO_PIN_RESET   ) )

#define __AD9854_SPI_SDO_SET    ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_9, GPIO_PIN_SET   ) )
#define __AD9854_SPI_SDO_CLR    ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_9, GPIO_PIN_RESET   ) )

#define __AD9854_SPI_SDIO_SET 	 ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_8, GPIO_PIN_SET   ) )
#define __AD9854_SPI_SDIO_CLR 	 ( HAL_GPIO_WritePin ( GPIOB, GPIO_PIN_8, GPIO_PIN_RESET   ) )


#define PHASE1	0x00	    // phase adjust register #1
#define PHASE2  0x01		// phase adjust register #2
#define FREQ1   0x02		// frequency tuning word 1
#define FREQ2   0x03		// frequency tuning word 2
#define DELFQ   0x04		// delta frequency word
#define UPDCK   0x05		// update clock
#define RAMPF   0x06		// ramp rate clock
#define CONTR   0x07		// control register
#define SHAPEI  0x08		// output shape key I mult
#define SHAPEQ  0x09		// output shape key Q mult
#define RAMPO   0x0A		// output shape key ramp rate
#define CDAC    0x0B		// QDAC
extern uint16_t add;
extern uint16_t step_up;
extern uint16_t amp_buf;
extern uint8_t modu_buf;

//**********************以下为系统时钟以及其相关变量设置**************************/
/* 
  此处根据自己的需要设置系统时钟以及与其相关的因子，一次需且只需开启一个
  CLK_Set为时钟倍频设置，可设置4~20倍倍频，但最大不能超过300MHZ
  Freq_mult_ulong和Freq_mult_doulle均为2的48次方除以系统时钟，一个为长整形，一个为双精度型
*/
//#define CLK_Set 7
//const uint32_t  Freq_mult_ulong  = 1340357;
//const double Freq_mult_doulle = 1340357.032;

//#define CLK_Set 9137431
//const uint32_t  Freq_mult_ulong  = 1042500;
//const double Freq_mult_doulle = 1042499.9137431;

// #define CLK_Set 8
// const uint32_t  Freq_mult_ulong  = 1172812;
// const double Freq_mult_doulle = 1172812.403;

#define CLK_Set 10
const uint32_t  Freq_mult_ulong  = 938250;
const double Freq_mult_doulle = 938249.9224;

uint8_t FreqWord[6];
uint16_t add = 0;
uint16_t step_up = 0;
uint32_t fund_fre_buf;
	
uint16_t amp_buf;
uint8_t modu_buf;
//====================================================================================
// 初始化程序区
//====================================================================================

/**
 * 设置AD9854与单片机连接的端口GPIO的初始化，完全可以通过CUBEMX来配置
 * 全部都设置为推完和上拉模式
 */
void AD9854_GPIO_Init() {
	GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_Initure.Pin = GPIO_PIN_4 | GPIO_PIN_5| GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_PULLUP;
	GPIO_Initure.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);


	GPIO_Initure.Pin = GPIO_PIN_5 | GPIO_PIN_6;
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Initure.Pull = GPIO_PULLUP;
	GPIO_Initure.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOE, &GPIO_Initure);

    GPIO_Initure.Pin = GPIO_PIN_13;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_Initure);
}
//====================================================================================
// AD9854驱动区
//====================================================================================
/**
 * Adata是8位的传输数据，该函数按照最高位开始，通过控制SDI位来进行传输
 */
void AD9854_WR_Byte(uint8_t Adata) {
	unsigned char  i;
	
	for(i = 8; i > 0; i--)
	{
		if(Adata & 0x80)				/// 1 --> SDI_SET
            __AD9854_SPI_SDIO_SET;
		else
            __AD9854_SPI_SDIO_CLR;
		Adata <<= 1;
//		HAL_Delay(0);
        __AD9854_WR_CLR;
        __AD9854_WR_SET;
	}
}

//====================================================================================
//函数名称:void AD9854_Init(void)
//函数功能:AD9854初始化
//入口参数:无
//出口参数:无
//====================================================================================
void AD9854_Init(void) {
    AD9854_GPIO_Init();
    __AD9854_SP_CLR;								 // 串行控制，SP = 0 --> 串行
    __AD9854_WR_CLR;
    __AD9854_UD_CLR;
    __AD9854_RT_SET;                // 复位AD9854
	HAL_Delay(0);
    __AD9854_RT_CLR;
    __AD9854_SPI_SDIO_CLR;
    __AD9854_RD_CLR;
	
	AD9854_WR_Byte(CONTR);
// 	AD9854_WR_Byte(0x10);							// 关闭比较器
	AD9854_WR_Byte(0x00);						// 打开比较器

	AD9854_WR_Byte(CLK_Set);					// 设置系统时钟倍频
	AD9854_WR_Byte(0x00);						// 设置系统为模式0，由外部更新
	AD9854_WR_Byte(0x60);

    __AD9854_UD_SET;                                // 更新AD9854输出
    __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void Freq_convert(long Freq)
//函数功能:正弦信号频率数据转换
//入口参数:Freq   需要转换的频率，取值从0~SYSCLK/2
//出口参数:无   但是影响全局变量FreqWord[6]的值
//说明：   该算法位多字节相乘算法，有公式FTW = (Desired Output Frequency × 2N)/SYSCLK
//         得到该算法，其中N=48，Desired Output Frequency 为所需要的频率，即Freq，SYSCLK
//         为可编程的系统时钟，FTW为48Bit的频率控制字，即FreqWord[6]
//====================================================================================
static void AD9854_Freq_convert(uint32_t freq) {
  uint32_t FreqBuf;
  uint32_t Temp=Freq_mult_ulong;

	uint8_t Array_Freq[4];			     //������Ƶ�����ӷ�Ϊ�ĸ��ֽ�
	Array_Freq[0] = (uint8_t)freq;
	Array_Freq[1] = (uint8_t)(freq >> 8);
	Array_Freq[2] = (uint8_t)(freq >> 16);
	Array_Freq[3] = (uint8_t)(freq >> 24);

	FreqBuf = Temp * Array_Freq[0];
	FreqWord[0] = FreqBuf;
	FreqBuf >>= 8;

	FreqBuf += (Temp * Array_Freq[1]);
	FreqWord[1] = FreqBuf;
	FreqBuf >>= 8;

	FreqBuf += (Temp*Array_Freq[2]);
	FreqWord[2] = FreqBuf;
	FreqBuf >>= 8;

	FreqBuf += (Temp * Array_Freq[3]);
	FreqWord[3] = FreqBuf;
	FreqBuf >>= 8;

	FreqWord[4] = FreqBuf;
	FreqWord[5] = FreqBuf>>8;
}  

//====================================================================================
//函数名称:void AD9854_SetSine(ulong Freq,uint Shape)
//函数功能:AD9854正弦波产生程序
//入口参数:Freq   频率设置，取值范围为0~(1/2)*SYSCLK
//         Shape  幅度设置. 为12 Bit,取值范围为(0~4095) ,取值越大,幅度越大
//出口参数:无
//====================================================================================
void AD9854_SINSet(uint32_t freq, uint16_t amp) {
	uint8_t count;
	uint8_t i = 0;

	AD9854_Freq_convert(freq);		              //Ƶ��ת��
    for(count = 6; count > 0; ) {
        if (i == 0)
            AD9854_WR_Byte(FREQ1);
        AD9854_WR_Byte(FreqWord[--count]);
        i++;
    }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
	
	AD9854_WR_Byte(SHAPEI);						  //����Iͨ������
	AD9854_WR_Byte(amp >> 8);
	AD9854_WR_Byte((uint8_t)(amp & 0xff));
	
	AD9854_WR_Byte(SHAPEQ);							//����Qͨ������
	AD9854_WR_Byte(amp >> 8);
	AD9854_WR_Byte((uint8_t)(amp & 0xff));

    __AD9854_UD_SET;                   //����AD9854���
    __AD9854_UD_CLR;

}

////====================================================================================
////函数名称:void Freq_doublt_convert(double Freq)
////函数功能:正弦信号频率数据转换
////入口参数:Freq   需要转换的频率，取值从0~SYSCLK/2
////出口参数:无   但是影响全局变量FreqWord[6]的值
////说明：   有公式FTW = (Desired Output Frequency × 2N)/SYSCLK得到该函数，
////         其中N=48，Desired Output Frequency 为所需要的频率，即Freq，SYSCLK
////         为可编程的系统时钟，FTW为48Bit的频率控制字，即FreqWord[6]
////注意：   该函数与上面函数的区别为该函数的入口参数为double，可使信号的频率更精确
////         谷雨建议在100HZ以下用本函数，在高于100HZ的情况下用函数void AD9854_Freq_convert(long Freq)
////====================================================================================
//void Freq_double_convert(double Freq)   
//{
//	uint32_t Low32;
//	uint16_t High16;
//  double Temp=Freq_mult_doulle;   	            //23ca99Ϊ2��48�η�����120M
//	Freq*=(double)(Temp);
////	1 0000 0000 0000 0000 0000 0000 0000 0000 = 4294967295
//	High16 = (int)(Freq/4294967295);                  //2^32 = 4294967295
//	Freq -= (double)High16*4294967295;
//	Low32 = (uint32_t)Freq;

//	FreqWord[0]=Low32;	     
//	FreqWord[1]=Low32>>8;
//	FreqWord[2]=Low32>>16;
//	FreqWord[3]=Low32>>24;
//	FreqWord[4]=High16;
//	FreqWord[5]=High16>>8;			
//} 

////====================================================================================
////函数名称:void AD9854_SetSine_double(double Freq,uint Shape)
////函数功能:AD9854正弦波产生程序
////入口参数:Freq   频率设置，取值范围为0~1/2*SYSCLK
////         Shape  幅度设置. 为12 Bit,取值范围为(0~4095)
////出口参数:无
////====================================================================================
//void AD9854_SetSine_double(double Freq,uint16_t Shape)
//{
//	uint8_t count=0;
//	uint8_t i=0;

//	Freq_double_convert(Freq);		           //Ƶ��ת��
//	 
//	for(count=6;count>0;)	                    //д��6�ֽڵ�Ƶ�ʿ�����  
//  {
//    if(i==0)
//			AD9854_WR_Byte(FREQ1);
//		AD9854_WR_Byte(FreqWord[--count]);
//		i++;
//  }
//	
//	AD9854_WR_Byte(SHAPEI);						  //����Iͨ������
//	AD9854_WR_Byte(Shape>>8);
//	AD9854_WR_Byte((uint8_t)(Shape&0xff));
//	
//	AD9854_WR_Byte(SHAPEQ);							//����Qͨ������
//	AD9854_WR_Byte(Shape>>8);
//	AD9854_WR_Byte((uint8_t)(Shape&0xff));

//	__AD9854_UD_SET;                    //����AD9854���
//  __AD9854_UD_CLR;
//}

//====================================================================================
//函数名称:void AD9854_InitFSK(void)
//函数功能:AD9854的FSK初始化
//入口参数:无
//出口参数:无
//====================================================================================
void AD9854_FSKInit(void) {
    AD9854_GPIO_Init();
    __AD9854_SP_CLR;
    __AD9854_WR_CLR;
    __AD9854_UD_CLR;
    __AD9854_RT_SET;
	HAL_Delay(0);
    __AD9854_RT_CLR;
    __AD9854_SPI_SDIO_CLR;
    __AD9854_RD_CLR;

	AD9854_WR_Byte(CONTR);
	AD9854_WR_Byte(0x10);
	AD9854_WR_Byte(CLK_Set);
	AD9854_WR_Byte(0x02);
	AD9854_WR_Byte(0x60);

    __AD9854_UD_SET;
    __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void AD9854_SetFSK(ulong Freq1,ulong Freq2)
//函数功能:AD9854的FSK设置
//入口参数:Freq1   FSK频率1
//         Freq2   FSK频率2
//出口参数:无
//====================================================================================
void AD9854_FSKSet(uint32_t Freq1, uint32_t Freq2) {
    uint8_t count=6;
	uint8_t i=0,j=0;

	const uint16_t Shape=4000;	      //��������. Ϊ12 Bit,ȡֵ��ΧΪ(0~4095)
	
	AD9854_Freq_convert(Freq1);               //Ƶ��ת��1
	
	for(count=6;count>0;)	          //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(i==0)
			AD9854_WR_Byte(FREQ1);
		AD9854_WR_Byte(FreqWord[--count]);
		i++;
  }
	
	AD9854_Freq_convert(Freq2);               //Ƶ��ת��2

	for(count=6;count>0;)	          //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(j==0)
			AD9854_WR_Byte(FREQ2);
		AD9854_WR_Byte(FreqWord[--count]);
		j++;
  }

	AD9854_WR_Byte(SHAPEI);						  //����Iͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));
	
	AD9854_WR_Byte(SHAPEQ);							//����Qͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));

    __AD9854_UD_SET;                    //����AD9854���
    __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void AD9854_InitBPSK(void)
//函数功能:AD9854的BPSK初始化
//入口参数:无
//出口参数:无
//====================================================================================
void AD9854_BPSKInit(void) {
    AD9854_GPIO_Init();
    __AD9854_SP_CLR;
    __AD9854_WR_CLR;                 // ������д���ƶ˿���Ϊ��Ч
    __AD9854_UD_CLR;
    __AD9854_RT_SET;                // ��λAD9854
    __AD9854_RT_CLR;
    __AD9854_SPI_SDIO_CLR;
    __AD9854_RD_CLR;

	AD9854_WR_Byte(CONTR);
	AD9854_WR_Byte(0x10);							//�رձȽ���
	AD9854_WR_Byte(CLK_Set);					//����ϵͳʱ�ӱ�Ƶ       
	AD9854_WR_Byte(0x08);							//����ϵͳΪģʽ0�����ⲿ����
	AD9854_WR_Byte(0x60);

    __AD9854_UD_SET;                //����AD9854���
    __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void AD9854_SetBPSK(uint Phase1,uint Phase2)
//函数功能:AD9854的BPSK设置
//入口参数:Phase1   调制相位1
//         Phase2	调制相位2
//出口参数:无
//说明：   相位为14Bit，取值从0~16383，谷雨建议在用本函数的时候将Phase1设置为0，
//         将Phase1设置为8192，180°相位
//====================================================================================
void AD9854_BPSKSet(uint16_t Phase1,uint16_t Phase2)
{
	uint8_t count;
  uint8_t i=0;
	const uint32_t Freq=60000;
  const uint16_t Shape=4000;

	AD9854_WR_Byte(PHASE1);										//������λ1
	AD9854_WR_Byte(Phase1>>8);
	AD9854_WR_Byte((uint8_t)(Phase1&0xff));

	AD9854_WR_Byte(PHASE2);										//������λ2
	AD9854_WR_Byte(Phase2>>8);
	AD9854_WR_Byte((uint8_t)(Phase2&0xff));
	AD9854_Freq_convert(Freq);                       //Ƶ��ת��

	for(count=6;count>0;)	                         //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(i==0)
			AD9854_WR_Byte(FREQ1);
		AD9854_WR_Byte(FreqWord[--count]);
		i++;
  }

	AD9854_WR_Byte(SHAPEI);						  			//����Iͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));
	
	AD9854_WR_Byte(SHAPEQ);										//����Qͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));

    __AD9854_UD_SET;                         //����AD9854���
    __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void AD9854_InitOSK(void)
//函数功能:AD9854的OSK初始化
//入口参数:无
//出口参数:无
//====================================================================================
void AD9854_InitOSK(void)
{
    __AD9854_SP_CLR;
    __AD9854_WR_CLR;                 // ������д���ƶ˿���Ϊ��Ч
    __AD9854_UD_CLR;
    __AD9854_RT_SET;                // ��λAD9854
	HAL_Delay(0);
    __AD9854_RT_CLR;
    __AD9854_SPI_SDIO_CLR;
    __AD9854_RD_CLR;

	AD9854_WR_Byte(CONTR);
	AD9854_WR_Byte(0x10);						//�رձȽ���
	AD9854_WR_Byte(CLK_Set);				//����ϵͳʱ�ӱ�Ƶ       
	AD9854_WR_Byte(0x00);						//����ϵͳΪģʽ0�����ⲿ����
	AD9854_WR_Byte(0x70);	          //����Ϊ�ɵ��ڷ��ȣ�ȡ����ֵ����,ͨ�������ڲ�����

    __AD9854_UD_SET;               //����AD9854���
    __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void AD9854_SetOSK(uchar RateShape)
//函数功能:AD9854的OSK设置
//入口参数: RateShape    OSK斜率,取值为4~255，小于4则无效
//出口参数:无
//====================================================================================
void AD9854_SetOSK(uint8_t RateShape)
{
	uint8_t count;
  uint8_t i=0;
	const uint32_t Freq=60000;			  //������Ƶ
  const uint16_t  Shape=4000;			//��������. Ϊ12 Bit,ȡֵ��ΧΪ(0~4095)

	AD9854_Freq_convert(Freq);                       //Ƶ��ת��

	for(count=6;count>0;)	                    //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(i==0)
			AD9854_WR_Byte(FREQ1);
		AD9854_WR_Byte(FreqWord[--count]);
		i++;
  }

	AD9854_WR_Byte(SHAPEI);						  		  //����Iͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));
	
	AD9854_WR_Byte(SHAPEQ);										//����Qͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));
       
	AD9854_WR_Byte(RAMPO);										//����OSKб��
	AD9854_WR_Byte(RateShape);			          //����OSKб��

    __AD9854_UD_SET;                         //����AD9854���
    __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void AD9854_InitAM(void)
//函数功能:AD9854的AM初始化
//入口参数:无
//出口参数:无
//====================================================================================
void AD9854_AMInit(void) {
    AD9854_GPIO_Init();
	uint8_t count;
  uint8_t i=0;
	const uint32_t Freq=800000;			 //������Ƶ

    __AD9854_SP_CLR;
    __AD9854_WR_CLR;                 // ������д���ƶ˿���Ϊ��Ч
    __AD9854_UD_CLR;
    __AD9854_RT_SET;                // ��λAD9854
	HAL_Delay(0);
    __AD9854_RT_CLR;
    __AD9854_SPI_SDIO_CLR;
    __AD9854_RD_CLR;

	AD9854_WR_Byte(CONTR);
	AD9854_WR_Byte(0x10);							//�رձȽ���
	AD9854_WR_Byte(CLK_Set);					//����ϵͳʱ�ӱ�Ƶ       
	AD9854_WR_Byte(0x00);							//����ϵͳΪģʽ0�����ⲿ����
	AD9854_WR_Byte(0x60);						  //����Ϊ�ɵ��ڷ��ȣ�ȡ����ֵ��?

	AD9854_Freq_convert(Freq);                            //Ƶ��ת��

	for(count=6;count>0;)	                         //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(i==0)
			AD9854_WR_Byte(FREQ1);
		AD9854_WR_Byte(FreqWord[--count]);
		i++;
  }

    __AD9854_UD_SET;                             //����AD9854���
    __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void AD9854_SetAM(uchar Shape)
//函数功能:AD9854的AM设置
//入口参数:Shape   12Bit幅度,取值从0~4095
//出口参数:无
//====================================================================================
void AD9854_AMSet(uint16_t Shape) {
	AD9854_WR_Byte(SHAPEI);						  				//����Iͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));
	
	AD9854_WR_Byte(SHAPEQ);											//����Qͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));

    __AD9854_UD_SET;                                   //����AD9854���
    __AD9854_UD_CLR;
}
//====================================================================================
//函数名称:void am_test (uint32_t fund_fre, uint8_t modulation)
//函数功能:AD9854的AM波形的调制频率和调制度的设置
//入口参数:fund_fre   调制频率 （0~100）
//				 modulation 调制度   （0~100）
//出口参数:无
//====================================================================================
void am_test (uint32_t fund_fre, uint8_t modulation)
{
	modu_buf = modulation;
	fund_fre_buf = fund_fre;	
	step_up = (float)fund_fre_buf * 65535 / 10000;
}
//====================================================================================
//函数名称:void AD9854_InitRFSK(void)
//函数功能:AD9854的RFSK初始化
//入口参数:无
//出口参数:无
//====================================================================================
void AD9854_RFSKInit(void) {
    AD9854_GPIO_Init();
    __AD9854_SP_CLR;
    __AD9854_WR_CLR;                 // ������д���ƶ˿���Ϊ��Ч
    __AD9854_UD_CLR;
    __AD9854_RT_SET;                // ��λAD9854
	HAL_Delay(0);
    __AD9854_RT_CLR;
    __AD9854_SPI_SDIO_CLR;
    __AD9854_RD_CLR;
//	AD9854_FDATA_Clr;

	AD9854_WR_Byte(CONTR);
	AD9854_WR_Byte(0x10);							//�رձȽ���
	AD9854_WR_Byte(CLK_Set);					//����ϵͳʱ�ӱ�Ƶ       
	AD9854_WR_Byte(0x24);							//����ϵͳΪģʽ0�����ⲿ����
	AD9854_WR_Byte(0x20);		        //����Ϊ�ɵ��ڷ��ȣ�ȡ����ֵ����,�ر�SincЧӦ	

    __AD9854_UD_SET;                   //����AD9854���
    __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void AD9854_SetRFSK(void)
//函数功能:AD9854的RFSK设置
//入口参数:Freq_Low          RFSK低频率	   48Bit
//         Freq_High         RFSK高频率	   48Bit
//         Freq_Up_Down		 步进频率	   48Bit
//		     FreRate           斜率时钟控制  20Bit
//出口参数:无
//注：     每两个脉冲之间的时间周期用下式表示（FreRate +1）*（System Clock ），一个脉冲,
//         频率 上升或者下降 一个步进频率
//====================================================================================
void AD9854_RFSKSet(uint32_t Freq_Low,uint32_t Freq_High,uint32_t Freq_Up_Down,uint32_t FreRate)
{
	uint8_t count=6;
	uint8_t i=0,j=0,k=0;
  const uint16_t  Shape=3600;			   //��������. Ϊ12 Bit,ȡֵ��ΧΪ(0~4095)

	AD9854_Freq_convert(Freq_Low);                             //Ƶ��1ת��

	for(count=6;count>0;)	                         //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(i==0)
			AD9854_WR_Byte(FREQ1);
		AD9854_WR_Byte(FreqWord[--count]);
		i++;
  }

	AD9854_Freq_convert(Freq_High);                             //Ƶ��2ת��

	for(count=6;count>0;)	                         //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(j==0)
			AD9854_WR_Byte(FREQ2);
		AD9854_WR_Byte(FreqWord[--count]);
		j++;
  }

	AD9854_Freq_convert(Freq_Up_Down);                             //����Ƶ��ת��

	for(count=6;count>0;)	                               //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(k==0)
			AD9854_WR_Byte(DELFQ);
		AD9854_WR_Byte(FreqWord[--count]);
		k++;
  }

	AD9854_WR_Byte(RAMPF);						  				//����б������
	AD9854_WR_Byte((uint8_t)((FreRate>>16)&0x0f));
	AD9854_WR_Byte((uint8_t)(FreRate>>8));
	AD9854_WR_Byte((uint8_t)FreRate);

	AD9854_WR_Byte(SHAPEI);						  				//����Iͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));
	
	AD9854_WR_Byte(SHAPEQ);											//����Qͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));

    __AD9854_UD_SET;                                //����AD9854���
    __AD9854_UD_CLR;
}
//====================================================================================
//函数名称:void  AD9854_InitChirp(void)
//函数功能:AD9854的Chirp初始化
//入口参数:无
//出口参数:无
//====================================================================================
void AD9854_ChirpInit(void) {
    AD9854_GPIO_Init();
    __AD9854_SP_CLR;
    __AD9854_WR_CLR;                 // ������д���ƶ˿���Ϊ��Ч
    __AD9854_UD_CLR;
    __AD9854_RT_SET;                // ��λAD9854
	HAL_Delay(0);
    __AD9854_RT_CLR;
    __AD9854_SPI_SDIO_CLR;
    __AD9854_RD_CLR;
	
	AD9854_WR_Byte(CONTR);
	AD9854_WR_Byte(0x10);	  			// �رձȽ���
	AD9854_WR_Byte(CLK_Set);	   		// ����ϵͳʱ�ӱ�Ƶ
	AD9854_WR_Byte(0x26);	        // ����ϵͳΪģʽ2�����ⲿ����,ʹ�����ǲ�ɨƵ����
	AD9854_WR_Byte(0x60);	        // ����Ϊ�ɵ��ڷ��ȣ�ȡ����ֵ����	
//	__AD9854_UD_SET;                                   //����AD9854���
//  __AD9854_UD_CLR;
}

//====================================================================================
//函数名称:void AD9854_SetChirp(unsigned long Freq_Low,unsigned long Freq_Up_Down,unsigned long FreRate)
//函数功能:AD9854的Chirp设置
//入口参数:Freq_Low          RFSK低频率	   48Bit
//         Freq_Up_Down		 步进频率	   48Bit
//		   FreRate           斜率时钟控制  20Bit
//出口参数:无
//注：     每两个脉冲之间的时间周期用下式表示（FreRate +1）*（System Clock ），一个脉冲,
//         频率 上升或者下降 一个步进频率
//====================================================================================
void AD9854_ChirpSet(unsigned long Freq_Low,unsigned long Freq_Up_Down,unsigned long FreRate) {
	uint8_t count=6;
	uint8_t i=0,k=0;
  const uint16_t  Shape=4000;			   //��������. Ϊ12 Bit,ȡֵ��ΧΪ(0~4095)

	AD9854_Freq_convert(Freq_Low);                             //Ƶ��1ת��

	for(count=6;count>0;)	                         //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(i==0)
			AD9854_WR_Byte(FREQ1);
		AD9854_WR_Byte(FreqWord[--count]);
		i++;
  }

	AD9854_Freq_convert(Freq_Up_Down);                             //����Ƶ��ת��

	for(count=6;count>0;)	                               //д��6�ֽڵ�Ƶ�ʿ�����  
  {
		if(k==0)
			AD9854_WR_Byte(DELFQ);
		AD9854_WR_Byte(FreqWord[--count]);
		k++;
  }

	AD9854_WR_Byte(RAMPF);						  				//����б������
	AD9854_WR_Byte((uint8_t)((FreRate>>16)&0x0f));
	AD9854_WR_Byte((uint8_t)(FreRate>>8));
	AD9854_WR_Byte((uint8_t)FreRate);

	AD9854_WR_Byte(SHAPEI);						  				//����Iͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));
	
	AD9854_WR_Byte(SHAPEQ);											//����Qͨ������
	AD9854_WR_Byte(Shape>>8);
	AD9854_WR_Byte((uint8_t)(Shape&0xff));

    __AD9854_UD_SET;                                //����AD9854���
    __AD9854_UD_CLR;
}
//====================================================================================
//函数名称:void Sweep_out (void)
//函数功能:扫频输出
//入口参数：无
//出口参数:无
//====================================================================================
void Sweep_out (void)
{
	uint32_t i=0,F=1;
 
	for(i=0;i<58;i++)
	{
			AD9854_SINSet(F ,4095);
	  	HAL_Delay(200);
		if (F < 1000)									// ���Ƶ��С��1kHz
					{
						F += 199;
					}
					else if (F< 10000)						// ���Ƶ��С��10kHz
					{
						F += 2000;
					}
					else if (F < 100000)					// ���Ƶ��С��100kHz
					{
						F += 20000;
					}
					else if (F < 1000000)					// ���Ƶ��С��1MHz
					{
						F += 200000;
					}
					else if (F < 10000000)				// ���Ƶ��С��10MHz
					{
						F += 2000000;
					}
					else 
					{
						F += 5000000;								// ���Ƶ��С��140MHz
					}

     if(F>=135000000) F=135000000;
	
  }
}
