#include "bsp_usart.hpp"
#include "stm32f1xx_it.h"
#include "main.h"
#include "usart.h"
#include "bsp_usart.hpp"
// #include "Monitor.hpp"
#include <string.h>
/*------------全局变量------------*/
extern UART_HandleTypeDef huart6;
extern DMA_HandleTypeDef hdma_usart6_rx;
extern DMA_HandleTypeDef hdma_usart6_tx;

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

#define RXBUFFERSIZE 512
#define TXBUFFERSIZE 512

uint8_t aRxBuffer[RXBUFFERSIZE];
uint16_t read_index=0;
static uint8_t aTxBuffer[TXBUFFERSIZE];

Pressure_Packet_t Current_Pressure;
uint8_t Pressure_Updated_Flag=0;
//采用状态机方式接收数据包
static uint8_t rx_byte_temp;        // 临时存接收到的 1 个字节
static uint8_t rx_buffer[18];       // 暂存正在组装的包
static uint8_t rx_count = 0;        // 目前收到了第几个字节

// 定义状态机的状态
typedef enum {
    STATE_WAIT_HEADER_1, // 等待 0xA5
    STATE_WAIT_HEADER_2, // 等待 0x5A
    STATE_RECEIVE_BODY   // 接收剩余数据
} Rx_State_t;
//初始化状态机状态
static Rx_State_t current_state = STATE_WAIT_HEADER_1;

// --- 初始化函数 ---
void Pressure_DMA_Init(UART_HandleTypeDef *huart)
{
    // 开启DMA接收，填入aRxBuffer
    if(HAL_UART_Receive_DMA(&huart1, aRxBuffer, RXBUFFERSIZE)!=HAL_OK)
    {
      Error_Handler();
    };
}
void Pressure_Process_Byte(uint8_t rx_byte_temp)
{
    // 状态机逻辑
    switch (current_state)
    {
    case STATE_WAIT_HEADER_1:
        if (rx_byte_temp == 0xA5) // 找到了第一字节
        {
            rx_buffer[0] = rx_byte_temp;
            current_state = STATE_WAIT_HEADER_2;
        }
        // 如果不是 0xA5，状态不变，继续等下一个字节
        break;

    case STATE_WAIT_HEADER_2:
        if (rx_byte_temp == 0x5A) // 找到了第二字节
        {
            rx_buffer[1] = rx_byte_temp;
            rx_count = 2; // 已经收了2字节(头)
            current_state = STATE_RECEIVE_BODY;
        }
        else 
        {
            // 这种情况下，如果当前字节是 0x5A，那它可能是新的头，否则重置
            if (rx_byte_temp == 0xA5)
                current_state = STATE_WAIT_HEADER_2; // 依然保留在等第2字节状态
            else
                current_state = STATE_WAIT_HEADER_1; // 彻底重置
        }
        break;

    case STATE_RECEIVE_BODY:
        rx_buffer[rx_count] = rx_byte_temp;
        rx_count++;

        // 这里的 18 是 sizeof(Pressure_Packet_t)
        if (rx_count >= 18) 
        {
            // 1. 包收完了，开始校验
            // 计算校验和：前16个字节累加
            uint16_t cal_sum = 0;
            for (int i = 0; i < 16; i++)
            {
                cal_sum += rx_buffer[i];
            }

            // 提取包里的校验和 (注意大小端，这里假设是小端直接转)
            Pressure_Packet_t *pTemp = (Pressure_Packet_t *)rx_buffer;

            if (cal_sum == pTemp->checksum)
            {
                // 2. 校验通过：把数据搬运到全局变量
                // 这样在外面读取时，永远读到的是校验通过的正确数据
                memcpy(&Current_Pressure, pTemp, sizeof(Pressure_Packet_t));
                Pressure_Updated_Flag = 1; 
            }
            
            // 3. 无论校验成不成功，都要重置状态，准备收下一包
            current_state = STATE_WAIT_HEADER_1;
            rx_count = 0;
        }
        break;
    }
    // 继续收下一个字节
}
// --- 任务循环函数 ---
void Pressure_Task_Loop(void)
{
	uint16_t write_index = RXBUFFERSIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx);
	while(read_index != write_index)
	{
		Pressure_Process_Byte(aRxBuffer[read_index]);
		read_index++;
		if(read_index >= RXBUFFERSIZE)
		{
			read_index = 0;
		}
	}
}


void USART_Init(void)
{
  USART1_Init();
//   USART6_Init();
}

void USART6_Init()
{
  __HAL_DMA_DISABLE_IT(&hdma_usart6_rx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart6_rx, DMA_IT_TC);  // 启用传输完成中断
  __HAL_DMA_DISABLE_IT(&hdma_usart6_tx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart6_tx, DMA_IT_TC);  // 启用传输完成中断

  // 开启DMA接收
  // HAL_UART_Receive_DMA(&huart6, aRxBuffer, 2);
  uint8_t message[] = "[INFO]USART6 Init Success!\r\n";
  USART_Transmit(&huart1, message, sizeof(message), USART_MODE_BLOCK);
};

void USART1_Init()
{
  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart1_rx, DMA_IT_TC);  // 启用传输完成中断
  __HAL_DMA_DISABLE_IT(&hdma_usart1_tx, DMA_IT_HT); // 禁用半传输中断
  __HAL_DMA_ENABLE_IT(&hdma_usart1_tx, DMA_IT_TC);  // 启用传输完成中断

  // 开启DMA接收
  // HAL_UART_Receive_DMA(&huart6, aRxBuffer, 2);
  uint8_t message[] = "[INFO]USART1 Init Success!\r\n";
  USART_Transmit(&huart1, message, sizeof(message), USART_MODE_BLOCK);
};

void USART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, enum USART_Mode mode)
{
  memcpy(aTxBuffer, pData, Size);
  switch (mode)
  {
  case USART_MODE_BLOCK:
    HAL_UART_Transmit(huart, aTxBuffer, Size, 100);
    break;
  case USART_MODE_DMA:
    HAL_UART_Transmit_DMA(huart, aTxBuffer, Size);
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

void USART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
  memcpy(pData, aRxBuffer, Size);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    HAL_UART_Receive_DMA(&huart1, aRxBuffer, 2);
  }
}