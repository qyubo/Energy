//
// Created by cosmosmount on 2025/8/30.
//

#ifndef RM26_BSP_SPI_HPP
#define RM26_BSP_SPI_HPP

#include "spi.h"

typedef enum
{
    SPI_BLOCK_MODE = 0,
    SPI_IT_MODE,
    SPI_DMA_MODE
} SPI_WORK_MODE;

/**
 * @brief  SPI: 发送数据
 * @param  hspi: SPI句柄
 * @param  data: 数据
 * @param  len: 数据长度
 * @param  mode: 工作模式
 * @todo 加入阻塞、中断、DMA模式，加入超时机制以及错误处理
 */
void SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *data, uint8_t len, SPI_WORK_MODE mode);

/**
 * @brief  SPI 读取数据
 * @param  hspi: SPI句柄
 * @param  pData: 数据
 * @param  len: 数据长度
 * @param  mode: 工作模式
 * @todo 加入阻塞、中断、DMA模式，加入超时机制以及错误处理
 */
void SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint8_t len, SPI_WORK_MODE mode);

#endif //RM26_BSP_SPI_HPP