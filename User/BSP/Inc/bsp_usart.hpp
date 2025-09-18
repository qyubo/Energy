#ifndef BSP_USART_HPP
#define BSP_USART_HPP

#include "main.h"
#include "string.h"
#include "stm32f4xx_hal_uart.h"

#define RXBUFFERSIZE 2u
#define TXBUFFERSIZE 2u
#define SBUS_RX_BUF_NUM 36u

enum USART_Mode
{
    USART_MODE_BLOCK = 0,
    USART_MODE_DMA = 1,
    USART_MODE_IT = 2
  };

/**
 * @brief 初始化串口。
 */
void USART_Init(void);

/**
 * @brief 初始化串口1。
 * @note 串口1用于调试信息输出。使用DMA发送和接收。
 */
void USART1_Init(void);

/**
 * @brief 初始化串口6。
 * @note 串口6用于和裁判系统通信。使用DMA发送和接收。
 */
void USART6_Init(void);

/**
 * @brief 初始化串口3。
 * @param rx1_buf DMA接收缓冲区1
 * @param rx2_buf DMA接收缓冲区2
 * @param dma_buf_num DMA缓冲区大小
 * @note 串口3用于接收遥控器数据。需要在初始化时传入DMA接收缓冲区1、DMA接收缓冲区2和DMA缓冲区大小，因此只能在remoteControl.c中调用。
 */
void USART3_Init(uint8_t *rx1_buf, uint8_t *rx2_buf, uint16_t dma_buf_num);

/**
 * @brief 串口发送数据。
 * @param huart 指向串口句柄的指针
 * @param pData 发送的数据
 * @param Size 数据长度
 * @param mode 发送模式
 */
void USART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, enum USART_Mode mode);

/**
 * @brief 串口接收数据。
 * @param huart 指向串口句柄的指针
 * @param pData 接收数据的缓冲区
 * @param Size 数据长度
 */
void USART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);

#endif //  __BSP_USART_H
