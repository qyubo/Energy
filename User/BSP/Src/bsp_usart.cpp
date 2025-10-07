#include "bsp_usart.hpp"
#include "stm32f4xx_it.h"
#include "ServiceRemoter.hpp"
#include "tx_api.h"

/*------------全局变量------------*/
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;

extern UART_HandleTypeDef huart6;
extern DMA_HandleTypeDef hdma_usart6_rx;
extern DMA_HandleTypeDef hdma_usart6_tx;

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

uint8_t SBUS_rx_buf[2][SBUS_RX_BUF_NUM];

static uint8_t UART1RxBuffer[RXBUFFERSIZE];

static uint8_t TxBuffer[TXBUFFERSIZE];

extern uint8_t dr16_rx[DR16_DATA_SIZE];
extern TX_SEMAPHORE RemoterThreadSem;

void USART_Init(void)
{
  USART1_Init();
  USART6_Init();
  USART3_Init(SBUS_rx_buf[0], SBUS_rx_buf[1], SBUS_RX_BUF_NUM);
}

void USART6_Init()
{
  __HAL_DMA_DISABLE_IT(&hdma_usart6_rx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart6_rx, DMA_IT_TC);  // 启用传输完成中断
  __HAL_DMA_DISABLE_IT(&hdma_usart6_tx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart6_tx, DMA_IT_TC);  // 启用传输完成中断

  // 开启DMA接收
  // HAL_UART_Receive_DMA(&huart6, UART6RxBuffer, 21);
};

void USART1_Init()
{
  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart1_rx, DMA_IT_TC);  // 启用传输完成中断
  __HAL_DMA_DISABLE_IT(&hdma_usart1_tx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart1_tx, DMA_IT_TC);  // 启用传输完成中断
};

void USART3_Init(uint8_t *rx1_buf, uint8_t *rx2_buf, uint16_t dma_buf_num)
{
  // 使能DMA窗口接收
  SET_BIT(huart3.Instance->CR3, USART_CR3_DMAR);
  // 使能空闲中断
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
  // 失效DMA
  __HAL_DMA_DISABLE(&hdma_usart3_rx);
  while (hdma_usart3_rx.Instance->CR & DMA_SxCR_EN)
  {
    __HAL_DMA_DISABLE(&hdma_usart3_rx);
  }

  hdma_usart3_rx.Instance->PAR = (uint32_t)&(USART3->DR);

  // 内存缓冲区1
  hdma_usart3_rx.Instance->M0AR = (uint32_t)(rx1_buf);
  // 内存缓冲区2
  hdma_usart3_rx.Instance->M1AR = (uint32_t)(rx2_buf);
  // 数据长度
  hdma_usart3_rx.Instance->NDTR = dma_buf_num;

  // 使能双缓冲区
  SET_BIT(hdma_usart3_rx.Instance->CR, DMA_SxCR_DBM);
  // 使能DMA
  __HAL_DMA_ENABLE(&hdma_usart3_rx);
}

void USART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, enum USART_Mode mode)
{
  memcpy(TxBuffer, pData, Size);
  switch (mode)
  {
  case USART_MODE_BLOCK:
    HAL_UART_Transmit(huart, TxBuffer, Size, 100);
    break;
  case USART_MODE_DMA:
    HAL_UART_Transmit_DMA(huart, TxBuffer, Size);
    break;
  case USART_MODE_IT:
    break;
  default:
    break;
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART6)
  {
    // HAL_UART_Receive_DMA(&huart6, VT03::Instance()->VT03_Data.ReceiveBuffer, 21);
    // HAL_UART_Receive_DMA(&huart6, UART6RxBuffer, 21);
    // if (UART6RxBuffer[0] == VT03_SOF_1 && UART6RxBuffer[0] == VT03_SOF_1)
    // {
    //   if (Verify_CRC16_Check_Sum((uint8_t*)&UART6RxBuffer, VT03_DATA_SIZE))
    //   {
    //     memcpy(VT03::Instance()->VT03_Data.ReceiveBuffer, UART6RxBuffer, VT03_DATA_SIZE);
    //     VT03::Instance()->AliveFlag++;
    //   }
    // }
  }
  if (huart->Instance == USART1)
  {
    HAL_UART_Receive_DMA(&huart1, UART1RxBuffer, 2);
  }
}

void USART3_IRQHandler(void)
{
    /* USER CODE BEGIN USART3_IRQn 0 */

    /* USER CODE END USART3_IRQn 0 */
    HAL_UART_IRQHandler(&huart3);
    /* USER CODE BEGIN USART3_IRQn 1 */
    if (huart3.Instance->SR & UART_FLAG_RXNE)
    {
        __HAL_UART_CLEAR_PEFLAG(&huart3);
    }
    else if (USART3->SR & UART_FLAG_IDLE)
    {
        static uint16_t current_rx_len = 0;

        __HAL_UART_CLEAR_PEFLAG(&huart3);

        if ((hdma_usart3_rx.Instance->CR & DMA_SxCR_CT) == RESET)
        {
            /* Current memory buffer used is Memory 0 */

            // disable DMA
            __HAL_DMA_DISABLE(&hdma_usart3_rx);

            // get receive data length, length = set_data_length - remain_length
            current_rx_len = SBUS_RX_BUF_NUM - hdma_usart3_rx.Instance->NDTR;

            // reset set_data_lenght
            hdma_usart3_rx.Instance->NDTR = SBUS_RX_BUF_NUM;

            // set memory buffer 1
            hdma_usart3_rx.Instance->CR |= DMA_SxCR_CT;

            // enable DMA
            __HAL_DMA_ENABLE(&hdma_usart3_rx);

            if (current_rx_len == DR16_DATA_SIZE)
            {
                memcpy(dr16_rx, SBUS_rx_buf[0], DR16_DATA_SIZE);
                tx_semaphore_put(&RemoterThreadSem);
            }
        }
        else
        {
            /* Current memory buffer used is Memory 1 */
            // disable DMA
            __HAL_DMA_DISABLE(&hdma_usart3_rx);

            // get receive data length, length = set_data_length - remain_length
            current_rx_len = SBUS_RX_BUF_NUM - hdma_usart3_rx.Instance->NDTR;

            // reset set_data_lenght
            hdma_usart3_rx.Instance->NDTR = SBUS_RX_BUF_NUM;

            // set memory buffer 0
            DMA1_Stream1->CR &= ~(DMA_SxCR_CT);

            // enable DMA
            __HAL_DMA_ENABLE(&hdma_usart3_rx);

            if (current_rx_len == DR16_DATA_SIZE)
            {
                memcpy(dr16_rx, SBUS_rx_buf[1], DR16_DATA_SIZE);
                tx_semaphore_put(&RemoterThreadSem);
            }
        }
    }
    /* USER CODE END USART3_IRQn 1 */
}