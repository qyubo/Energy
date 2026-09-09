//
// Created by cosmosmount on 2025/9/1.
//

#ifndef RM26_H7_BSP_WS2812_HPP
#define RM26_H7_BSP_WS2812_HPP

#include "spi.h"

extern SPI_HandleTypeDef hspi6; // SPI句柄

void WS2812_Ctrl(uint8_t r, uint8_t g, uint8_t b);

#endif //RM26_H7_BSP_WS2812_HPP