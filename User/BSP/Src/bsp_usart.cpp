#include "bsp_usart.hpp"
#include "XRobot.hpp"
#include "referee.hpp"
// #include "ServiceRemoter.hpp"

/*------------全局变量------------*/
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;

extern UART_HandleTypeDef huart6;
extern DMA_HandleTypeDef hdma_usart6_rx;
extern DMA_HandleTypeDef hdma_usart6_tx;

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

/**
 * @brief  Configures the USART.
 * @param  None
 * @retval None
 */

void USART_Init()
{
  __HAL_DMA_DISABLE_IT(&hdma_usart6_rx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart6_rx, DMA_IT_TC);  // 启用传输完成中断
  __HAL_DMA_DISABLE_IT(&hdma_usart6_tx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart6_tx, DMA_IT_TC);  // 启用传输完成中断

  // 开启DMA接收
  // HAL_UART_Receive_DMA(&huart6, UART6RxBuffer, 21);
  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart1_rx, DMA_IT_TC);  // 启用传输完成中断
  __HAL_DMA_DISABLE_IT(&hdma_usart1_tx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart1_tx, DMA_IT_TC);  // 启用传输完成中断

  // 使能DMA窗口接收
  SET_BIT(huart3.Instance->CR3, USART_CR3_DMAR);
  // 使能空闲中断
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
  // 失效DMA
  __HAL_DMA_DISABLE(&hdma_usart3_rx);
  __HAL_DMA_ENABLE(&hdma_usart3_rx);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart1)
  {
    // //清空cache
    // DMA_Cache_PrepareForReceive(USART1RxBuffer, 1);
    //
    // Referee::Instance()->Referee_Rx_Queue.Push(USART1RxBuffer[0]);
    // HAL_UART_Receive_DMA(&huart1, USART1RxBuffer, 1);
  }
}
