#ifndef HIT_ADC_H
#define HIT_ADC_H

#include <stdint.h>

#define ADC_COUNT 5
#define CHANNELS_PER_ADC 2
#define CHANNEL_COUNT (ADC_COUNT * CHANNELS_PER_ADC)
#define DMA_VALUES_PER_ADC 256
#define VALUES_PER_BLOCK (DMA_VALUES_PER_ADC / 2)  
#define FRAMES_PER_BLOCK (VALUES_PER_BLOCK / CHANNELS_PER_ADC)  // 每个DMA块中每个ADC的帧数

extern volatile uint16_t adc_dma_buffer[ADC_COUNT][DMA_VALUES_PER_ADC];

uint8_t HitADC_Init(void);
uint8_t HitADC_IsRunning(void);
uint8_t HitADC_ReadBlock(uint16_t block[FRAMES_PER_BLOCK][CHANNEL_COUNT]);

#endif 
