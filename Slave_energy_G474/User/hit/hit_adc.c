#include "hit_adc.h"

#include "adc.h"
#include "tim.h"

// DMA buffer for ADC values
volatile uint16_t adc_dma_buffer[ADC_COUNT][DMA_VALUES_PER_ADC] = {0}; 
static ADC_HandleTypeDef *const adc_handles[ADC_COUNT] = {
    &hadc1, &hadc2, &hadc3, &hadc4, &hadc5
};

// 记录采集状态
static uint8_t adc_running = 0;  

// 检查ADC采集是否正在运行
uint8_t HitADC_IsRunning(void) {
    return adc_running;
}

// 记录每个ADC的DMA传输块计数, 前半块DMA写后半块CPU读
static volatile uint32_t adc_block_count[ADC_COUNT] = {0};
// 它记录CPU上次成功读取的块编号，避免同一块被处理多次
static uint32_t adc_read_count = 0;


// 停止ADC采集与DMA传输
static void HitADC_StopDMA(uint8_t started_count) {
    for (uint8_t adc_index = 0; adc_index < started_count; adc_index++) {
        HAL_ADC_Stop_DMA(adc_handles[adc_index]);  
    }
}

// 初始化ADC采集与DMA传输
uint8_t HitADC_Init(void) {
    if (adc_running != 0) {
        return 1;
    }
    for (uint8_t adc_index = 0; adc_index < ADC_COUNT; adc_index++) {
        // 启动ADC校准 ADC对应句柄地址，单端输入校准
        if (HAL_ADCEx_Calibration_Start(adc_handles[adc_index], ADC_SINGLE_ENDED) != HAL_OK) {
            return 0;
        } 
    }
    for (uint8_t adc_index = 0; adc_index < ADC_COUNT; adc_index++) {
        if (HAL_ADC_Start_DMA(adc_handles[adc_index], (uint32_t *)adc_dma_buffer[adc_index], DMA_VALUES_PER_ADC) != HAL_OK) {
            HitADC_StopDMA(adc_index);
            return 0;
        }
    }
    if (HAL_TIM_Base_Start(&htim6) != HAL_OK) {
        HitADC_StopDMA(ADC_COUNT);
        return 0;
    }
    adc_running = 1;
    return 1;
}

// ADC转换半完成回调函数
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
    for (uint8_t adc_index = 0; adc_index < ADC_COUNT; adc_index++) {
        if (hadc == adc_handles[adc_index]) {
            adc_block_count[adc_index]++;
            break;
        }
    }
}

// ADC转换完成回调函数
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    for (uint8_t adc_index = 0; adc_index < ADC_COUNT; adc_index++) {
        if (hadc == adc_handles[adc_index]) {
            adc_block_count[adc_index]++;
            break;
        }
    }
}

// 检查指定块编号的DMA传输是否完成
static uint8_t HitADC_BlockReady(uint32_t block_count) {
    for (uint8_t adc_index = 0; adc_index < ADC_COUNT; adc_index++) {
        DMA_HandleTypeDef *dma = adc_handles[adc_index]->DMA_Handle;
        // 获取DMA剩余传输数据量
        uint32_t remaining = __HAL_DMA_GET_COUNTER(dma);
        // 获取DMA的半满和全满标志
        uint32_t flags = __HAL_DMA_GET_HT_FLAG_INDEX(dma) | __HAL_DMA_GET_TC_FLAG_INDEX(dma);

        if (adc_block_count[adc_index] != block_count || __HAL_DMA_GET_FLAG(dma, flags) != 0U) {
            return 0;
        }

        if (block_count & 1U) {
            if (remaining == 0U || remaining > VALUES_PER_BLOCK) {
                return 0;
            }
        } else {
            if (remaining <= VALUES_PER_BLOCK) {
                return 0;
            }
        }
    }

    return 1;
    
}

uint8_t HitADC_ReadBlock(uint16_t block[FRAMES_PER_BLOCK][CHANNEL_COUNT]) {
    // 未启动采集返回0
    if (adc_running == 0) {
        return 0;
    }
    // 先取ADC1的完成块计数，作为本次待检查的块编号
    uint32_t block_count = adc_block_count[0];
    // 与上次相同，没有新的，返回0
    if (block_count == adc_read_count) {
        return 0;
    }
    // 检查所有ADC的块计数是否一致，若不一致，说明有ADC还未完成采集，返回0
    for (uint8_t ADC_index = 0; ADC_index < ADC_COUNT; ADC_index++) {
        if (adc_block_count[ADC_index] != block_count) {
            return 0;
        }
    }
    // 取计数的最低位判断奇偶，奇数表示前半块已写完，读取前半；偶数表示后半块已写完，读取后半
    uint16_t offset = (block_count & 1U) ? 0 : VALUES_PER_BLOCK;
    // 确认现在可以安全读取这半块
    if (HitADC_BlockReady(block_count) == 0) {
        return 0;
    }
    for (uint16_t frame_index = 0; frame_index < FRAMES_PER_BLOCK; frame_index++) {
        for (uint8_t ADC_index = 0; ADC_index < ADC_COUNT; ADC_index++) {
            for (uint8_t channel_index = 0; channel_index < CHANNELS_PER_ADC; channel_index++) {
                uint16_t value = adc_dma_buffer[ADC_index][offset + frame_index * CHANNELS_PER_ADC + channel_index];
                block[frame_index][ADC_index * CHANNELS_PER_ADC + channel_index] = value;
            }
        }
    }
    // 确认复制过程中，DMA没有开始覆盖这半块
    if (HitADC_BlockReady(block_count) == 0) {
        return 0;
    }
    uint8_t result = ((uint32_t)(block_count - adc_read_count) == 1U) ? 1U : 2U;

    adc_read_count = block_count;
    // 返回0：无数据；1：连续块；2：有效块但此前有跳块
    return result;
}
