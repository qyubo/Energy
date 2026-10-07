#ifndef HIT_ADC_H
#define HIT_ADC_H

#include <stdint.h>

#define HIT_ADC_COUNT 5
#define HIT_CHANNELS_PER_ADC 2
#define HIT_CHANNEL_COUNT (HIT_ADC_COUNT * HIT_CHANNELS_PER_ADC)
#define HIT_DMA_VALUES_PER_ADC 256U

extern volatile uint16_t hit_adc_dma_buffer[HIT_ADC_COUNT][HIT_DMA_VALUES_PER_ADC];

uint8_t HitADC_Init(void);
uint8_t HitADC_IsRunning(void);


#endif 
