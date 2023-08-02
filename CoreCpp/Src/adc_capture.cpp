#include "adc_capture.h"

#include "main_cpp.h"

// adc_capture v1.2 2023/7/15
// adc_capture v1.3 2023/7/28

static uint8_t __adc_flag = 0;
static ADC_HandleTypeDef* __hadc1 = nullptr;
static TIM_HandleTypeDef *__htim1 = nullptr;
static ADC_HandleTypeDef* __hadc2 = nullptr;
static TIM_HandleTypeDef *__htim2 = nullptr;
static ADC_HandleTypeDef* __hadc3 = nullptr;
/**
 * Use only one hadc and timer
 * @param hadc1
 * @param htim1
 */
void ADC_CAPTURE_Init(ADC_HandleTypeDef* hadc1, TIM_HandleTypeDef *htim1) {
    __hadc1 = hadc1;
    __htim1 = htim1;
}

/**
 * Use two hadc and two timer
 * @param hadc1
 * @param htim1
 * @param hadc2
 * @param htim2
 */
void ADC_CAPTURE_Init(ADC_HandleTypeDef* hadc1, TIM_HandleTypeDef *htim1,
                      ADC_HandleTypeDef* hadc2, TIM_HandleTypeDef *htim2) {
    __hadc1 = hadc1;
    __htim1 = htim1;

    __hadc2 = hadc2;
    __htim2 = (htim2 == nullptr) ? htim1 : htim2;
}

/**
 * external clk trigger adc3
 * @param hadc3
 */
void ADC_EXTCAPTURE_Init(ADC_HandleTypeDef* hadc3, I2C_HandleTypeDef *hi2c) {
    SI5351_Init(hi2c);
    __hadc3 = hadc3;
}

static void ADC_CAPTURE_Start(uint16_t* adc_value, uint32_t sample_length,
                       ADC_HandleTypeDef* hadc, TIM_HandleTypeDef *htim) {
    HAL_ADC_Start_DMA(hadc, (uint32_t *)adc_value, sample_length);
    HAL_TIM_Base_Start(htim);
}


static void ADC_CAPTURE_Start(uint16_t* adc_value, uint32_t sample_length,
                              ADC_HandleTypeDef* hadc) {
    // external clk trigger adc3
    HAL_ADC_Start_DMA(hadc, (uint32_t *)adc_value, sample_length);
    // si5351

}

static void ADC_CAPTURE_Stop(ADC_HandleTypeDef* hadc, TIM_HandleTypeDef *htim) {
	HAL_TIM_Base_Stop(htim);
	HAL_ADC_Stop_DMA(hadc);
}

static void ADC_CAPTURE_Stop(ADC_HandleTypeDef* hadc) {
    // external clk trigger adc3
    HAL_ADC_Stop_DMA(hadc);
}


void ADC1_CAPTURE_Callback() {
    __adc_flag = 1;
    ADC_CAPTURE_Stop(__hadc1, __htim1);
}

void ADC2_CAPTURE_Callback() {
    __adc_flag = 1;
    ADC_CAPTURE_Stop(__hadc2, __htim2);
}

void ADC3_EXTCAPTURE_Callback() {
    // external clk trigger adc3
    __adc_flag = 1;
    ADC_CAPTURE_Stop(__hadc3);
}

/*
 * 通过频率得到合适的num_samples和address_step_rate
 * psc: 16bit 分频
 * cnt: 16bit 计数值
 */
static void ADC_Capture_FindFactor(uint32_t fs, uint32_t &psc, uint32_t &cnt) {
    const uint32_t TIM_CLK = 240e6;
    const double FAC_NUM = (double)TIM_CLK / (double)fs;
//    static const uint32_t PSC_LIST[] =
//            {2, 4, 5, 8, 10, 16, 20, 25, 32, 40, 50, 64,
//             80, 100, 125, 128, 160, 200, 250, 320, 400,
//             500, 625, 640, 800, 1000, 1250, 1600, 2000,
//             2500, 3125, 3200, 4000, 5000, 6250, 8000,10000,
//             12500, 15625, 16000, 20000, 25000, 31250, 40000,
//             50000, 62500};    // 250e6的因素（时钟频率240e6我tm找250e6的因数。。。）

    static const uint32_t PSC_LIST[] = {
            2, 3, 4, 5, 6, 8, 10, 12, 15, 16, 20, 24, 25,
            30, 32, 40, 48, 50, 60, 64, 75, 80, 96, 100,
            120, 125, 128, 150, 160, 192, 200, 240, 250, 256,
            300, 320, 375, 384, 400, 480, 500, 512, 600, 625,
            640, 750, 768, 800, 960, 1000, 1024, 1200, 1250, 1280,
            1500, 1536, 1600, 1875, 1920, 2000, 2400, 2500, 2560,
            3000, 3072, 3125, 3200, 3750, 3840, 4000, 4800, 5000,
            5120, 6000, 6250, 6400, 7500, 7680, 8000, 9375, 9600,
            10000, 12000, 12500, 12800, 15000, 15360, 15625, 16000,
            18750, 19200, 20000, 24000, 25000, 25600, 30000, 31250,
            32000, 37500, 38400, 40000, 46875, 48000, 50000, 60000,
            62500, 64000
    };
    const int LEN_PSC_LIST = sizeof(PSC_LIST) / sizeof(PSC_LIST[0]);
    uint32_t psc_temp = 0;
    uint32_t cnt_temp = 0;
    double fs_temp = 0;
    double error_min = 1e16;
    for (int i = 0; i < LEN_PSC_LIST; ++i) {
        psc_temp = PSC_LIST[i];
        cnt_temp = (uint32_t)round( FAC_NUM / (double)psc_temp );
        if (cnt_temp > 65536) {
            continue;
        }
        fs_temp = (double)TIM_CLK / (double)(psc_temp * cnt_temp);
        double error = abs(fs_temp - fs);
        if ( error < error_min ) {
            error_min = error;
            psc = psc_temp;
            cnt = cnt_temp;
        }
    }
}

static void ADC_CAPTURE_SettingFreq(uint32_t fs, TIM_HandleTypeDef *htim) {
    uint32_t psc = 0;
    uint32_t cnt = 0;
    ADC_Capture_FindFactor(fs, psc, cnt);
    __HAL_TIM_SET_PRESCALER(htim, psc - 1);
    __HAL_TIM_SET_AUTORELOAD(htim, cnt - 1);
}

static void ADC_CAPTURE_SettingFreq(uint32_t fs) {
    // external clk trigger adc3
    // si5351
    SI5351_SetCLK0((double)fs);    // channel 0 fs
}
/*
 * fs最大能设置到7e6也就是7Mhz，但是要超频到ADC时钟要超160Mhz
 */
void ADC1_CAPTURE_Capture(uint16_t* adc_value, uint32_t sample_length, uint32_t fs) {
    ADC_CAPTURE_SettingFreq(fs, __htim1);
    ADC_CAPTURE_Start(adc_value, sample_length, __hadc1, __htim1);

    while (__adc_flag == 0) {
        // waiting finished
    }
    __adc_flag = 0;
}

void ADC2_CAPTURE_Capture(uint16_t* adc_value, uint32_t sample_length, uint32_t fs) {
    ADC_CAPTURE_SettingFreq(fs, __htim2);
    ADC_CAPTURE_Start(adc_value, sample_length, __hadc2, __htim2);

    while (__adc_flag == 0) {
        // waiting finished
    }
    __adc_flag = 0;
}

void ADC3_EXTCAPTURE_Capture(uint16_t* adc_value, uint32_t sample_length, uint32_t fs) {
    // external clk trigger adc3
    ADC_CAPTURE_SettingFreq(fs);
    ADC_CAPTURE_Start(adc_value, sample_length, __hadc3);

    while (__adc_flag == 0) {
        // waiting finished
    }
    __adc_flag = 0;
}






















