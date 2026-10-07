#include "hit_adc.h"

#include "adc.h"
#include "tim.h"

volatile uint16_t hit_adc_dma_buffer[HIT_ADC_COUNT][HIT_DMA_VALUES_PER_ADC] = {0}; // DMA buffer for ADC values

static ADC_HandleTypeDef *const hit_adc_handles[HIT_ADC_COUNT] = {
    &hadc1, &hadc2, &hadc3, &hadc4, &hadc5
};

static uint8_t hit_adc_running = 0;  // 记录采集状态

static void HitADC_StopDMA(uint8_t started_count) {
    for (uint8_t adc_index = 0; adc_index < started_count; adc_index++) {
        HAL_ADC_Stop_DMA(hit_adc_handles[adc_index]);
    }
};

uint8_t HitADC_Init(void) {
    if (hit_adc_running != 0) {
        return 1;
    }
    for (uint8_t adc_index = 0; adc_index < HIT_ADC_COUNT; adc_index++) {
        if (HAL_ADCEx_Calibration_Start(hit_adc_handles[adc_index], ADC_SINGLE_ENDED) != HAL_OK) {
            return 0;
        } 
    }
    for (uint8_t adc_index = 0; adc_index < HIT_ADC_COUNT; adc_index++) {
        if (HAL_ADC_Start_DMA(hit_adc_handles[adc_index], (uint32_t *)hit_adc_dma_buffer[adc_index], HIT_DMA_VALUES_PER_ADC) != HAL_OK) {
            HitADC_StopDMA(adc_index);
            return 0;
        }
    }
    if (HAL_TIM_Base_Start(&htim6) != HAL_OK) {
        HitADC_StopDMA(HIT_ADC_COUNT);
        return 0;
    }
    hit_adc_running = 1;
    return 1;
}

uint8_t HitADC_IsRunning(void) {
    return hit_adc_running;
}