/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************.
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "dma.h"
#include "stm32f1xx_hal.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "WS2812.h"
#include "can_portocal_second.hpp"
#include "Flag_State.h"
#include "bsp_can.hpp"
#include "bsp_usart.hpp"
#include "bsp.hpp"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define WS1 33
#define WS0 12
#define BOARD_NUMBER 1//TODO:修改为对应板子编号，0~4
#define PRESS_THRESHOLD 80//TODO:检测修改阈值
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// 状态标记数组，对应10个环，防止按住不放时重复触发
// 0: 未触发, 1: 已触发
static uint8_t sensor_active_flags[10] = {0};
static uint8_t last_work_state=255;//用来记录上一次指令
int record=0;
int used_time=0;
// char one_ring=1;
// char two_ring=1;
// char three_ring=1;
// char four_ring=1;
// char five_ring=1;
// char six_ring=1;
// char seven_ring=1;
// char eight_ring=1;
// char nine_ring=1;
// char ten_ring=1;
int hit_ring[10]={1,1,1,1,1,1,1,1,1,1};//锁定环，1为未击打，0为已击打
Board_info_t Board_info[5];
Board_Order_info_t Board_Order_info[5];
Big_Fu_info_t Big_Fu_info[5];
uint8_t can_rx_Data[8]={0};
uint8_t can_Dafu_data[8]={0};

Pressure_Packet_t adc_data_debug;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
//发出一块板的是否击中和击中环数
void Process_Pressure_Data(void)
{
	if (Pressure_Updated_Flag == 0) return; 
    Pressure_Updated_Flag = 0;
    //HINT去掉了待击打状态才处理数据，这样可以检测是否错误击打
  // if(Board_Order_info[BOARD_NUMBER].Board_Work!=11)return;//只有在待击打状态下才处理压力数据
  uint8_t max_pressure = 0;
  int8_t max_index = -1;
	for(int i=0;i<10;i++)
	{
		uint8_t current_pressure = Current_Pressure.adc_data[i];
    memcpy(adc_data_debug.adc_data,Current_Pressure.adc_data,sizeof(Current_Pressure.adc_data));//debug的
    if(current_pressure <= PRESS_THRESHOLD && sensor_active_flags[i] == 1)
    {
      sensor_active_flags[i] = 0; // 传感器恢复未触发状态，解锁
		
}else if (current_pressure > PRESS_THRESHOLD && sensor_active_flags[i] == 0)//超过阈值且没有被锁定
		{


      sensor_active_flags[i] = 1; // 锁住这个传感器防止连发
      if(current_pressure >max_pressure)
      {
        max_pressure = current_pressure;
        max_index = i;
      }//寻优并记录环数
	}
		
}
if(max_index != -1)
{
  switch (max_index)//寻找实际该亮的环数
  {
case 0:
max_index = 4;
break;
case 1:
max_index = 3;
break;
case 2:
max_index = 2;
break;
case 3:
max_index = 1;
break;
case 4:
max_index = 0;
break;
case 5:
max_index = 9;
break;
case 6:
max_index = 8;
break;
case 7:
max_index = 7;
break;
case 8:
max_index = 6;
break;
case 9:
max_index = 5;
break;
default:
break;
  }  
    hit_ring[max_index]=0;//该环已被击打
			// 触发事件处理代码：特定环变成0，特定板子的击打次数+1，状态置1
			Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
      Big_Fu_info[BOARD_NUMBER].Hit_LED_state = max_index+1;//记录击中环数
			Board_Info_Tx(can_Dafu_data,Big_Fu_info);//发送给主板
      //HINT:加入了顺手清零
      Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 0;
}
}



/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_CAN_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	CAN_Init();
	Big_Fu_info_Init(Big_Fu_info);
	Board_Order_info_Init(Board_Order_info);
	// WS2812_close_all();
	extern UART_HandleTypeDef huart1; 
	Pressure_DMA_Init(&huart1);
	HAL_Delay(10);
  uint8_t test_msg[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  CAN_Transmit(&hcan, CAN_BOARD_ID_2_F, test_msg, 8);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//           WS_Write_RGB_TEN_RINGS(200,0,0);//击打成功十环
// HAL_Delay(1000);
// WS2812_close_all();
// HAL_Delay(1000);
    Pressure_Task_Loop();
		Process_Pressure_Data();
    
		// Board_Order_info[BOARD_NUMBER].Board_Work=2;//测试代码  

    uint8_t current_cmd = Board_Order_info[BOARD_NUMBER].Board_Work;
		if(current_cmd!=last_work_state)
    {
      switch(current_cmd)
      {
        case 0:
          WS2812_close_all();
          for(int i=0;i<10;i++)sensor_active_flags[i]=0;//解锁传感器
          break;
        case 1:
          WS_Write_RGB_ONE_RINGS(200,0,0);//击打成功十环
          break;
        case 2:
          WS_Write_RGB_TWO_RINGS(200,0,0);//击打成功十环
          break;
        case 3:
          WS_Write_RGB_THREE_RINGS(200,0,0);//击打成功十环
          break;
        case 4:
          WS_Write_RGB_FOUR_RINGS(200,0,0);//击打成功十环
          break;     
        case 5:
          WS_Write_RGB_FIVE_RINGS(200,0,0);//击打成功十环
          break;
        case 6:
          WS_Write_RGB_SIX_RINGS(200,0,0);//击打成功十环
          break;
        case 7:
          WS_Write_RGB_SEVEN_RINGS(200,0,0);//击打成功十环
          break;
        case 8:
          WS_Write_RGB_EIGHT_RINGS(200,0,0);//击打成功十环
          break;
        case 9:
          WS_Write_RGB_NINE_RINGS(200,0,0);//击打成功十环
          break; 
        case 10:
          WS_Write_RGB_TEN_RINGS(200,0,0);//击打成功十环
          break;   
        case 11:
          WS_Write_Cross(200,0,0);//待击打十字
          break;

      }
      last_work_state=current_cmd;
    }
    HAL_Delay(1);
	}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
