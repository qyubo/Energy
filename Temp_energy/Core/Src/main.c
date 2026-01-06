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
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "WS2812.h"
#include "can_portocal_second.hpp"
#include "Flag_State.h"
#include "bsp_can.hpp"
#include "bsp.hpp"
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define WS1 33
#define WS0 12
#define BOARD_NUMBER 1
#define SWITCH_PIN_0 GPIO_PIN_0
#define SWITCH_PIN_1 GPIO_PIN_1
#define SWITCH_PIN_2 GPIO_PIN_2
#define SWITCH_PIN_3 GPIO_PIN_3
#define SWITCH_PIN_4 GPIO_PIN_4
#define SWITCH_PIN_5 GPIO_PIN_5
#define SWITCH_PIN_12 GPIO_PIN_12
#define SWITCH_PIN_13 GPIO_PIN_13
#define SWITCH_PIN_14 GPIO_PIN_14
#define SWITCH_PIN_15 GPIO_PIN_15
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
int record=0;
int used_time=0;
char one_ring=1;
char two_ring=1;
char three_ring=1;
char four_ring=1;
char five_ring=1;
char six_ring=1;
char seven_ring=1;
char eight_ring=1;
char nine_ring=1;
char ten_ring=1;
Board_info_t Board_info[5];
Board_Order_info_t Board_Order_info[5];
Big_Fu_info_t Big_Fu_info[5];
uint8_t can_rx_Data[8]={0};
uint8_t can_Dafu_data[8]={0};
static int useTime=0;
int hasExecuted=0;
typedef enum {
    STABLE_HIGH,
    JITTER_HIGH,
    STABLE_LOW,
    JITTER_LOW
} Switch_State;
Switch_State current_state_pin_0 = STABLE_HIGH;
Switch_State current_state_pin_1 = STABLE_HIGH;
Switch_State current_state_pin_2 = STABLE_HIGH;
Switch_State current_state_pin_3 = STABLE_HIGH;
Switch_State current_state_pin_4 = STABLE_HIGH;
Switch_State current_state_pin_5 = STABLE_HIGH;
Switch_State current_state_pin_12 = STABLE_HIGH;
Switch_State current_state_pin_13 = STABLE_HIGH;
Switch_State current_state_pin_14 = STABLE_HIGH;
Switch_State current_state_pin_15 = STABLE_HIGH;
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
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    bool current_switch_level;
    switch (GPIO_Pin) {
    case SWITCH_PIN_0:
        current_switch_level = HAL_GPIO_ReadPin(GPIOB, SWITCH_PIN_0);
        switch (current_state_pin_0) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_0 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_0 = JITTER_LOW;
            } else {
                current_state_pin_0 = STABLE_HIGH;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_0 = JITTER_HIGH;
            }
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_0 = JITTER_HIGH;
            } else {
                current_state_pin_0 = STABLE_LOW;
                // ??????????????,?????????
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
                eight_ring = 0;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info);
                __HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_0);
            }
            break; 
        }
        break;
    case SWITCH_PIN_1:
        current_switch_level = HAL_GPIO_ReadPin(GPIOB, SWITCH_PIN_1); 
        switch (current_state_pin_1) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_1 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_1 = JITTER_LOW;
            } else {
                current_state_pin_1 = STABLE_HIGH;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_1 = JITTER_HIGH;
            }
						
						
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_1 = JITTER_HIGH;
            } else {
                current_state_pin_1 = STABLE_LOW;
                // ?????????
                seven_ring = 0;
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info);
                __HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_1);
            }
            break;
        }
        break;
    case SWITCH_PIN_2:
        current_switch_level = HAL_GPIO_ReadPin(GPIOB, SWITCH_PIN_2);
        switch (current_state_pin_2) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_2 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_2 = JITTER_LOW;
            } else {
                current_state_pin_2 = STABLE_HIGH;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_2 = JITTER_HIGH;
            }
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_2 = JITTER_HIGH;
            } else {
                current_state_pin_2 = STABLE_LOW;
                // ?????????
                six_ring = 0;
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info);
                __HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_2);
            }
            break;
        }
        break;
    case SWITCH_PIN_3:
        current_switch_level = HAL_GPIO_ReadPin(GPIOB, SWITCH_PIN_3);
        switch (current_state_pin_3) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_3 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_3 = JITTER_LOW;
            } else {
                current_state_pin_3 = STABLE_HIGH;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_3 = JITTER_HIGH;
            }
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_3 = JITTER_HIGH;
            } else {
                current_state_pin_3 = STABLE_LOW;
                // ?????????
                one_ring = 0;
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info); 							
							__HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_3);
            }
            break;
        }
        break;
    case SWITCH_PIN_4:
        current_switch_level = HAL_GPIO_ReadPin(GPIOA, SWITCH_PIN_4);
        switch (current_state_pin_4) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_4 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_4 = JITTER_LOW;
            } else {
                current_state_pin_4 = STABLE_HIGH;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_4 = JITTER_HIGH;
            }
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_4 = JITTER_HIGH;
            } else {
                current_state_pin_4 = STABLE_LOW;
                // ?????????
                ten_ring = 0;
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info);
                __HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_4);
            }
            break;
        }
        break;
    case SWITCH_PIN_5:
        current_switch_level = HAL_GPIO_ReadPin(GPIOA, SWITCH_PIN_5);
        switch (current_state_pin_5) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_5 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_5 = JITTER_LOW;
            } else {
                current_state_pin_5 = STABLE_HIGH;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_5 = JITTER_HIGH;
            }
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_5 = JITTER_HIGH;
            } else {
                current_state_pin_5 = STABLE_LOW;
                // ?????????
                nine_ring = 0;
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info);
                __HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_5);
            }
            break;
        }
        break;
    case SWITCH_PIN_12:
        current_switch_level = HAL_GPIO_ReadPin(GPIOB, SWITCH_PIN_12);
        switch (current_state_pin_12) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_12 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_12 = JITTER_LOW;
            } else {
                current_state_pin_12 = STABLE_LOW;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_12 = JITTER_HIGH;
            }
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_12 = JITTER_HIGH;
            } else {
                current_state_pin_12 = STABLE_LOW;
                // ?????????
                five_ring = 0;
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info);
                __HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_12);
            }
            break;
        }
        break;
    case SWITCH_PIN_13:
        current_switch_level = HAL_GPIO_ReadPin(GPIOB, SWITCH_PIN_13);
        switch (current_state_pin_13) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_13 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_13 = JITTER_LOW;
            } else {
                current_state_pin_13 = STABLE_HIGH;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_13 = JITTER_HIGH;
            }
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_13 = JITTER_HIGH;
            } else {
                current_state_pin_13 = STABLE_LOW;
                four_ring = 0;
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info);
                __HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_13);
            }
            break;
        }
        break;
    case SWITCH_PIN_14:
        current_switch_level = HAL_GPIO_ReadPin(GPIOB, SWITCH_PIN_14);
        switch (current_state_pin_14) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_14 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_14 = JITTER_LOW;
            } else {
                current_state_pin_14 = STABLE_HIGH;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_14 = JITTER_HIGH;
            }
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_14 = JITTER_HIGH;
            } else {
                current_state_pin_14 = STABLE_LOW;
                // ?????????
                three_ring = 0;
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info);
                __HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_14);
            }
            break;
        }
        break;
    case SWITCH_PIN_15:
        current_switch_level = HAL_GPIO_ReadPin(GPIOA, SWITCH_PIN_15);
        switch (current_state_pin_15) {
        case STABLE_HIGH:
            if (!current_switch_level) {
                current_state_pin_15 = JITTER_LOW;
            }
            break;
        case JITTER_HIGH:
            if (!current_switch_level) {
                current_state_pin_15 = JITTER_LOW;
            } else {
                current_state_pin_15 = STABLE_HIGH;
            }
            break;
        case STABLE_LOW:
            if (current_switch_level) {
                current_state_pin_15 = JITTER_HIGH;
            }
            break;
        case JITTER_LOW:
            if (current_switch_level) {
                current_state_pin_15 = JITTER_HIGH;
            } else {
                current_state_pin_15 = STABLE_LOW;
                // ?????????
                two_ring = 0;
                record = 1;
                Big_Fu_info[BOARD_NUMBER].hit_time += 1;
                Big_Fu_info[BOARD_NUMBER].Single_Hit_State = 1;
							Board_Info_Tx(can_Dafu_data,Big_Fu_info);
                __HAL_GPIO_EXTI_CLEAR_IT(SWITCH_PIN_15);
            }
            break;
        }
        break;
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
  /* USER CODE BEGIN 2 */
	CAN_Init();
	Big_Fu_info_Init(Big_Fu_info);
	Board_Order_info_Init(Board_Order_info);
	WS2812_close_all();
	HAL_Delay(10);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

//    Board_RX(CAN_BOARD_ID_1_F,Board_Order_info,can_rx_Data,Board_info[BOARD_NUMBER]);
	  Board_RX(CAN_BOARD_ID_2_F,Board_Order_info,can_rx_Data,Board_info[BOARD_NUMBER]);
//		Board_RX(CAN_BOARD_ID_3_F,Board_Order_info,can_rx_Data,Board_info[BOARD_NUMBER]);
//		Board_RX(CAN_BOARD_ID_4_F,Board_Order_info,can_rx_Data,Board_info[BOARD_NUMBER]);
//		Board_RX(CAN_BOARD_ID_5_F,Board_Order_info,can_rx_Data,Board_info[BOARD_NUMBER]);
		// Board_Order_info[BOARD_NUMBER].Board_Work=1;
		// Board_Order_info[BOARD_NUMBER].Energy_machine_state=1;//测试代码
		if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
		{
			used_time=0;
			record=0;
			Big_Fu_info_Init(Big_Fu_info);
			WS2812_close_all();
			HAL_Delay(10);
			WS_Write_RGB_Frame_up(0,0,0);
			HAL_Delay(10);
			WS_Write_RGB_Frame_down(0,0,0);	
      HAL_Delay(10);			
			WS2812_close_Rectangle();
			HAL_Delay(10);
			one_ring=1;
			two_ring=1;
			three_ring=1;
			four_ring=1;
			five_ring=1;
			six_ring=1;
			seven_ring=1;
			eight_ring=1;
			nine_ring=1;
			ten_ring=1;
			hasExecuted=0;
			useTime=0;
			current_state_pin_0 = STABLE_HIGH;
      current_state_pin_1 = STABLE_HIGH;
      current_state_pin_2 = STABLE_HIGH;
      current_state_pin_3 = STABLE_HIGH;
      current_state_pin_4 = STABLE_HIGH;
      current_state_pin_5 = STABLE_HIGH;
      current_state_pin_12 = STABLE_HIGH;
      current_state_pin_13 = STABLE_HIGH;
      current_state_pin_14 = STABLE_HIGH;
      current_state_pin_15 = STABLE_HIGH;
		}
		if(Board_Order_info[BOARD_NUMBER].Board_Work==1&&record==0)
		{
			if(used_time==0){
			used_time=1;
			WS_Write_Cross(100,0,0);
			HAL_Delay(10);
			}
			WS_Write_arrow_flow_LED(100,0,0);
		}
		if(Board_Order_info[BOARD_NUMBER].Board_Work==1&&record==1)
		{
			if(hasExecuted==0){
			hasExecuted=1;
			WS_Write_RGB_Frame_up(50,0,0);
			HAL_Delay(10);
			WS_Write_RGB_Frame_down(50,0,0);
			HAL_Delay(10);		

			}						
		}
    if(one_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			{
				 useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1)
					 {
					 WS_Write_RGB_ONE_RINGS(100,0,0);
					 HAL_Delay(10);
						 Board_Info_Tx(can_Dafu_data,Big_Fu_info);
					 }
				 }
			 }
		 }
		}
    if(two_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			 {
				   useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1)
					 {
					 WS_Write_RGB_TWO_RINGS(100,0,0);
					 HAL_Delay(10);
//			     WS_Write_Rectangle_down(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_mid(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_up(50,0,0);
//			     HAL_Delay(10);							 
					 }
				 }
			 }
		 }
		}     
    if(three_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			 {
				 useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1){
					 WS_Write_RGB_THREE_RINGS(100,0,0);
					 HAL_Delay(10);
//			     WS_Write_Rectangle_down(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_mid(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_up(50,0,0);
//			     HAL_Delay(10);							 
					 }
				 }
			 }
		 }
		}
    if(four_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			 {
				 useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1){
					 WS_Write_RGB_FOUR_RINGS(100,0,0);
					 HAL_Delay(10);
//			     WS_Write_Rectangle_down(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_mid(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_up(50,0,0);
//			     HAL_Delay(10);							 
					 }
				 }
			 }
		 }
		}
    if(five_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			 {
				 useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1){
					 WS_Write_RGB_FIVE_RINGS(100,0,0);
					 HAL_Delay(10);
//			     WS_Write_Rectangle_down(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_mid(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_up(50,0,0);
//			     HAL_Delay(10);							 
					 }
				 }
			 }
		 }
		}		
    if(six_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			 {
				 useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1){
					 WS_Write_RGB_SIX_RINGS(100,0,0);
					 HAL_Delay(10);
//			     WS_Write_Rectangle_down(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_mid(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_up(50,0,0);
//			     HAL_Delay(10);							 
					 }
				 }
			 }
		 }
		}	
    if(seven_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			 {
				 useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1){
					 WS_Write_RGB_SEVEN_RINGS(100,0,0);
					 HAL_Delay(10);
//			     WS_Write_Rectangle_down(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_mid(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_up(50,0,0);
//			     HAL_Delay(10);							 
					 }
				 }
			 }
		 }
		}		
    if(eight_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			 {
				 useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1){
					 WS_Write_RGB_EIGHT_RINGS(100,0,0);
					 HAL_Delay(10);
//			     WS_Write_Rectangle_down(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_mid(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_up(50,0,0);
//			     HAL_Delay(10);							 
					 }
				 }
			 }
		 }
		}		
    if(nine_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			 {
				 useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1){
					 WS_Write_RGB_NINE_RINGS(100,0,0);
					 HAL_Delay(10);
//			     WS_Write_Rectangle_down(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_mid(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_up(50,0,0);
//			     HAL_Delay(10);							 
					 }
				 }
			 }
		 }
		}	
    if(ten_ring==0)
		{
			if(Big_Fu_info[BOARD_NUMBER].hit_time<=1)
			{
			if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==0)
			 {
				 useTime=0;
				   record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==1)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
					 WS2812_close_all();
				 }
			if(Big_Fu_info[BOARD_NUMBER].Hit_LED_state==0&&Big_Fu_info[BOARD_NUMBER].Single_Hit_State==1&&Board_Order_info[BOARD_NUMBER].Board_Work==1)
			{
				useTime++;
				if(useTime==1)
				{
				WS_Write_RGB_ONE_RINGS(100,0,0);
				HAL_Delay(10);
				}
			}
			 }
			 if(Board_Order_info[BOARD_NUMBER].Energy_machine_state==2)
			 {
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==0)
				 {
					 useTime=0;
					 record=0;
					 Big_Fu_info_Init(Big_Fu_info);
				 WS2812_close_all();}
				 if(Board_Order_info[BOARD_NUMBER].Board_Work==1)
				 {
					 useTime++;
					 if(useTime==1){
					 WS_Write_RGB_TEN_RINGS(100,0,0);
					 HAL_Delay(10);
//			     WS_Write_Rectangle_down(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_mid(50,0,0);
//			     HAL_Delay(10);
//			     WS_Write_Rectangle_up(50,0,0);
//			     HAL_Delay(10);							 
					 }
				 }
			 }
		 }
		}		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
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
