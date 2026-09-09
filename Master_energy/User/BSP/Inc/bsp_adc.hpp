//
// Created by cosmosmount on 2025/8/30.
//

#ifndef RM26_BSP_ADC_HPP
#define RM26_BSP_ADC_HPP

#include "adc.h"

/**
 * @brief  初始化
 */
void ADC_Init(void);

/**
 * @brief  启动，只是对HAL库函数的形式上的封装
 * @retval 返回电压值(float)
 */
uint16_t* ADC_Start(void);

#endif //RM26_BSP_ADC_HPP