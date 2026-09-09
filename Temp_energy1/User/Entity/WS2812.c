#include "WS2812.h"
#include <stdint.h>
#include <string.h>
uint16_t send_Buf[220 *24+224]={0};
/* The F103 has limited RAM, so the animation reuses the existing DMA buffer. */
static WS2812_ArrowMode_t arrow_mode = ARROW_OFF;
static uint8_t arrow_dirty = 0U;
static volatile uint8_t arrow_dma_busy = 0U;
static uint8_t arrow_phase = 0U;
static uint8_t arrow_red = 0U;
static uint8_t arrow_green = 0U;
static uint8_t arrow_blue = 0U;
static uint32_t arrow_last_tick = 0U;
//uint16_t send_Buf_rectangle[50*24+224]={0};
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

uint32_t WS2812_color(  // 将RGB颜色值转换为24位的颜色数据，格式为GRB
	uint8_t red,
	uint8_t green,
	uint8_t blue)
{
	 return (green<<16)|(red<<8)|blue;
}

void SetPixelColor(uint16_t n,uint8_t red,uint8_t green,uint8_t blue)
{
	for(int i=0;i<24;i++)
	{
		if((((WS2812_color(red, green, blue) >> i) & 0X000001)==1))
		{//24bits最右位放入buf的最后面，WS2812要求从左向右发送
		send_Buf[24*n+23-i]=WS1;
		}
		else
		{
		send_Buf[24*n+23-i]=WS0;
		}
	}
}

//new
static void WS2812_SetPixelToBuffer(uint16_t *buffer,
	uint16_t n, uint8_t red, uint8_t green, uint8_t blue)
{
	uint32_t color = WS2812_color(red, green, blue);

	for(uint8_t i = 0U; i < 24U; i++)
	{
		buffer[24U * n + 23U - i] =
			((color >> i) & 0x000001U) ? WS1 : WS0;
	}
}

/* Convert a physical row/column to the serial LED number (serpentine wiring). */
static uint16_t WS2812_ArrowXYToIndex(uint8_t row, uint8_t col){
	if((row % 2) == 0)
	{
		return row * ARROW_COLS + col;
	}

	return row * ARROW_COLS + (ARROW_COLS - 1 - col);
}

/* Encode all 165 LEDs as black and leave the reset slots at zero duty. */
static void WS2812_ArrowClearBuffer(void)
{
	memset(send_Buf, 0, sizeof(send_Buf));

	for(uint16_t i = 0U; i < ARROW_LED_NUM * 24U; i++)
	{
		send_Buf[i] = WS0;
	}
}

/* All users of send_Buf must wait for either DMA reader to finish. */
uint8_t WS2812_DisplayReady(void)
{
    if (arrow_dma_busy) return 0U;
    if (htim2.hdma[TIM_DMA_ID_CC2] != NULL &&
        HAL_DMA_GetState(htim2.hdma[TIM_DMA_ID_CC2]) == HAL_DMA_STATE_BUSY)
        return 0U;
    return 1U;
}

static HAL_StatusTypeDef WS2812_ArrowSendBuffer(void)
{
    arrow_dma_busy = 1U;
    HAL_StatusTypeDef status = HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1,
        (uint32_t *)send_Buf, ARROW_DMA_SIZE);
    if (status != HAL_OK) arrow_dma_busy = 0U;
    return status;
}

void WS2812_ArrowSetMode(WS2812_ArrowMode_t mode,
    uint8_t red, uint8_t green, uint8_t blue)
{
    arrow_mode = mode;
    arrow_red = red;
    arrow_green = green;
    arrow_blue = blue;
    arrow_phase = 0U;
    arrow_dirty = 1U;
}

void WS2812_ArrowStart(uint8_t red, uint8_t green, uint8_t blue)
{
    WS2812_ArrowSetMode(ARROW_FLOW_RIGHT, red, green, blue);
}

void WS2812_ArrowShowLeftHalfRed(uint8_t red)
{
    WS2812_ArrowSetMode(ARROW_LEFT_HALF_RED, red, 0U, 0U);
}

void WS2812_ArrowStop(void)
{
    WS2812_ArrowSetMode(ARROW_OFF, 0U, 0U, 0U);
}

void WS2812_ArrowTask(void)
{
    uint32_t now = HAL_GetTick();
    if (!WS2812_DisplayReady()) return;
    if (!arrow_dirty)
    {
        if (arrow_mode != ARROW_FLOW_RIGHT) return;
        if ((uint32_t)(now - arrow_last_tick) < ARROW_FRAME_MS) return;
    }

    WS2812_ArrowClearBuffer();
    for (uint8_t row = 0U; row < ARROW_ROWS; ++row)
    {
        /* Rows 0..4 shift 0,1,2,1,0: the chevron points right. */
        uint8_t row_offset = row <= 2U ? row : 4U - row;
        uint8_t offset = (row_offset + arrow_phase) % 4U;
        for (uint8_t col = 0U; col < ARROW_COLS; ++col)
        {
            uint8_t lit = 0U;
            if (arrow_mode == ARROW_FLOW_RIGHT)
                lit = ((col + 4U - offset) % 4U) < 2U;
            else if (arrow_mode == ARROW_LEFT_HALF_RED)
                lit = col < ARROW_LEFT_COLS;
            if (lit)
                WS2812_SetPixelToBuffer(send_Buf,
                    WS2812_ArrowXYToIndex(row, col),
                    arrow_red, arrow_green, arrow_blue);
        }
    }

    if (WS2812_ArrowSendBuffer() == HAL_OK)
    {
        arrow_dirty = 0U;
        arrow_last_tick = now;
        if (arrow_mode == ARROW_FLOW_RIGHT)
            arrow_phase = (arrow_phase + 1U) % 4U;
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	if((htim->Instance == TIM1) &&
		(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1))
	{
		HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
		arrow_dma_busy = 0U;
	}
}
//new_end

void WS2812_Write_all()
{
	 for(uint16_t n=0;n<220;n++)
	 {
		 SetPixelColor(n,50,0,0);
	 }	
	 	 HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
}//外圈外一般灯亮不起来
 void WS2812_close_all()
 {
	 memset(send_Buf,0,sizeof(send_Buf));
	 for(uint16_t n=0;n<220;n++)
	 {
		 SetPixelColor(n,0,0,0);
	 }
	 HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	//  HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t*)send_Buf,220*24+224);//HINT
 }

 
	void WS_Write_RGB_ONE_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
	{
		WS2812_close_all();
		HAL_Delay(10);
		for(uint16_t n=0;n<4;n++)
		{
			SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	
	void WS_Write_RGB_TWO_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
	{
		WS2812_close_all();
		HAL_Delay(10);
		for(uint16_t n=4;n<12;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	
	
	void WS_Write_RGB_THREE_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
	{
		WS2812_close_all();
		HAL_Delay(10);
		for(uint16_t n=12;n<24;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	
	
		void WS_Write_RGB_FOUR_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
{
		WS2812_close_all();
	  HAL_Delay(10);
		for(uint16_t n=24;n<40;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	
	
		void WS_Write_RGB_FIVE_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
	{
		WS2812_close_all();
		HAL_Delay(10);
		for(uint16_t n=40;n<60;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	
	
		void WS_Write_RGB_SIX_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
	{
		WS2812_close_all();
		HAL_Delay(10);
		for(uint16_t n=60;n<84;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	
	
		void WS_Write_RGB_SEVEN_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
	{
		WS2812_close_all();
		HAL_Delay(10);
		for(uint16_t n=84;n<112;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	
	
		void WS_Write_RGB_EIGHT_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
	{
		WS2812_close_all();
		HAL_Delay(10);
		for(uint16_t n=112;n<144;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	
	
		void WS_Write_RGB_NINE_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
	{
		WS2812_close_all();
		HAL_Delay(10);
		for(uint16_t n=144;n<180;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	
	
		void WS_Write_RGB_TEN_RINGS(uint8_t n_R,uint8_t n_G,uint8_t n_B)
	{
		WS2812_close_all();
		HAL_Delay(10);
		for(uint16_t n=180;n<220;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		  HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
	}
	

void WS2812_close_Rectangle()
{
	memset(send_Buf,0,sizeof(send_Buf));
	for(uint16_t n=0;n<220;n++)
	 {
		 SetPixelColor(n,0,0,0);
	 }
	 HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t *)send_Buf,220*24+224);
	//  memset(send_Buf,0,sizeof(send_Buf));
}

void WS_Write_Rectangle_up(uint8_t n_R,uint8_t n_G,uint8_t n_B)
{
	memset(send_Buf,0,sizeof(send_Buf));
	for(uint16_t n=0;n<234;n++)
		{
			SetPixelColor(n,n_R,n_G,n_B);
		}
	 HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t *)send_Buf,234*24+224);
}

void WS_Write_Cross(uint8_t n_R,uint8_t n_G,uint8_t n_B)
{
	WS2812_close_all();
	// for(uint16_t n=12;n<24;n++)
	// 	{
	// 	  SetPixelColor(n,n_R,n_G,n_B);
	// 	}
	// 电源对测
	SetPixelColor(60,n_R,n_G,n_B);
	SetPixelColor(83,n_R,n_G,n_B);
	SetPixelColor(84,n_R,n_G,n_B);
	SetPixelColor(111,n_R,n_G,n_B);
	SetPixelColor(112,n_R,n_G,n_B);
	SetPixelColor(143,n_R,n_G,n_B);
	SetPixelColor(180,n_R,n_G,n_B);
	SetPixelColor(219,n_R,n_G,n_B);
	//面向电源右侧
	SetPixelColor(77,n_R,n_G,n_B);
	SetPixelColor(78,n_R,n_G,n_B);
	SetPixelColor(104,n_R,n_G,n_B);
	SetPixelColor(105,n_R,n_G,n_B);
	SetPixelColor(135,n_R,n_G,n_B);
	SetPixelColor(136,n_R,n_G,n_B);
	SetPixelColor(209,n_R,n_G,n_B);
	SetPixelColor(210,n_R,n_G,n_B);
	//面向电源内侧
	SetPixelColor(71,n_R,n_G,n_B);
	SetPixelColor(72,n_R,n_G,n_B);
	SetPixelColor(97,n_R,n_G,n_B);
	SetPixelColor(98,n_R,n_G,n_B);
	SetPixelColor(127,n_R,n_G,n_B);
	SetPixelColor(128,n_R,n_G,n_B);
	SetPixelColor(199,n_R,n_G,n_B);
	SetPixelColor(200,n_R,n_G,n_B);
	//面向电源左侧	
	SetPixelColor(65,n_R,n_G,n_B);
	SetPixelColor(66,n_R,n_G,n_B);
	SetPixelColor(90,n_R,n_G,n_B);
	SetPixelColor(91,n_R,n_G,n_B);
	SetPixelColor(119,n_R,n_G,n_B);
	SetPixelColor(120,n_R,n_G,n_B);
	SetPixelColor(189,n_R,n_G,n_B);
	SetPixelColor(190,n_R,n_G,n_B);


  	for(uint16_t n=4;n<12;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
		//此乃最内层
		// for(uint16_t n=12;n<24;n++)
		// {
		//   SetPixelColor(n,n_R,n_G,n_B);
		// }
	for(uint16_t n=40;n<61;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
	for(uint16_t n=144;n<180;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
	HAL_Delay(50);
	HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)send_Buf,220*24+224);
}

void WS_Write_RGB_Frame_up(uint8_t n_R, uint8_t n_G, uint8_t n_B)
{
	memset(send_Buf,0,sizeof(send_Buf));
	for(uint16_t n=0;n<300;n++)
	{
		SetPixelColor(n,n_R,n_G,n_B);
	}
	HAL_TIM_PWM_Start_DMA(&htim3,TIM_CHANNEL_1,(uint32_t *)send_Buf,300*24+224);
}


void WS_Write_RGB_Frame_down(uint8_t n_R, uint8_t n_G, uint8_t n_B)
{
	memset(send_Buf,0,sizeof(send_Buf));
	for(uint16_t n=0;n<300;n++)
	{
		SetPixelColor(n,n_R,n_G,n_B);
	}
  HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_4,(uint32_t*)send_Buf,300*24+224);	
}
//��ͷ��ˮ����ƺ���
void WS_Write_arrow_flow_LED(uint8_t n_R, uint8_t n_G, uint8_t n_B)
{
//	memset(send_Buf,0,sizeof(send_Buf));
	WS2812_close_Rectangle();  
	HAL_Delay(10);	
   for(int n=0;n<31;n+=4)
	{
		SetPixelColor(n,n_R,n_G,n_B);
		SetPixelColor(n+1,n_R,n_G,n_B);
		SetPixelColor(n+68,n_R,n_G,n_B);
		SetPixelColor(n+69,n_R,n_G,n_B);
		SetPixelColor(n+132,n_R,n_G,n_B);
		SetPixelColor(n+133,n_R,n_G,n_B);
	}
	for(int i=28;i>=0;i-=4)
	{
		SetPixelColor(63-i,n_R,n_G,n_B);
		SetPixelColor(64-i,n_R,n_G,n_B);
		SetPixelColor(129-i,n_R,n_G,n_B);
		SetPixelColor(130-i,n_R,n_G,n_B);
	}
	HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t *)send_Buf,234*24+224);
	HAL_Delay(50);
 memset(send_Buf,0,sizeof(send_Buf));
	WS2812_close_Rectangle();
	HAL_Delay(10);
   for(int n=0;n<31;n+=4)
	{
		SetPixelColor(n+1,n_R,n_G,n_B);
		SetPixelColor(n+2,n_R,n_G,n_B);
		SetPixelColor(n+69,n_R,n_G,n_B);
		SetPixelColor(n+70,n_R,n_G,n_B);
		SetPixelColor(n+133,n_R,n_G,n_B);
		SetPixelColor(n+134,n_R,n_G,n_B);
	}	
		for(int i=28;i>=0;i-=4)
	{
		SetPixelColor(62-i,n_R,n_G,n_B);
		SetPixelColor(63-i,n_R,n_G,n_B);
		SetPixelColor(128-i,n_R,n_G,n_B);
		SetPixelColor(129-i,n_R,n_G,n_B);
	}
	// for(int i=165;i<220;i++)
	// {
	// 	SetPixelColor(i,0,0,0);
	// }//HINT
	HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t *)send_Buf,234*24+224);
	HAL_Delay(50);
	memset(send_Buf,0,sizeof(send_Buf));//HINT
	WS2812_close_Rectangle();
	HAL_Delay(10);	
		for(int i=28;i>=0;i-=4)
	{
		SetPixelColor(61-i,n_R,n_G,n_B);
		SetPixelColor(62-i,n_R,n_G,n_B);
		SetPixelColor(127-i,n_R,n_G,n_B);
		SetPixelColor(128-i,n_R,n_G,n_B);
	}
   for(int n=0;n<31;n+=4)
	{
		SetPixelColor(n+2,n_R,n_G,n_B);
		SetPixelColor(n+3,n_R,n_G,n_B);
		SetPixelColor(n+70,n_R,n_G,n_B);
		SetPixelColor(n+71,n_R,n_G,n_B);
		SetPixelColor(n+134,n_R,n_G,n_B);
		SetPixelColor(n+135,n_R,n_G,n_B);
	}		
		SetPixelColor(66,n_R,n_G,n_B);
	HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t *)send_Buf,234*24+224);
	HAL_Delay(50);
	memset(send_Buf,0,sizeof(send_Buf));//HINT
	WS2812_close_Rectangle();
	HAL_Delay(10);	
   for(int n=0;n<31;n+=4)
	{
		SetPixelColor(n+3,n_R,n_G,n_B);
		SetPixelColor(n+4,n_R,n_G,n_B);
		SetPixelColor(n+71,n_R,n_G,n_B);
		SetPixelColor(n+72,n_R,n_G,n_B);
		SetPixelColor(n+135,n_R,n_G,n_B);
		SetPixelColor(n+136,n_R,n_G,n_B);
	}	
	SetPixelColor(100,0,0,0);
		for(int i=28;i>=0;i-=4)
	{
		SetPixelColor(60-i,n_R,n_G,n_B);
		SetPixelColor(61-i,n_R,n_G,n_B);
		SetPixelColor(126-i,n_R,n_G,n_B);
		SetPixelColor(127-i,n_R,n_G,n_B);
	}
	SetPixelColor(65,n_R,n_G,n_B);
	SetPixelColor(66,n_R,n_G,n_B);
	SetPixelColor(67,n_R,n_G,n_B);
	SetPixelColor(131,n_R,n_G,n_B);
	HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t *)send_Buf,234*24+224);
	HAL_Delay(50);
}
void WS2812_close_Frame_up()
{
		memset(send_Buf,0,sizeof(send_Buf));
	for(uint16_t n=0;n<234;n++)
	{
		SetPixelColor(n,0,0,0);
	}
	HAL_TIM_PWM_Start_DMA(&htim3,TIM_CHANNEL_1,(uint32_t *)send_Buf,234*24+224);
}
void WS_Write_Rectangle_down(uint8_t n_R,uint8_t n_G,uint8_t n_B)
{
		memset(send_Buf,0,sizeof(send_Buf));
	for(uint16_t n=0;n<234;n++)
		{
			SetPixelColor(n,n_R,n_G,n_B);
		}
	 HAL_TIM_PWM_Start_DMA(&htim3,TIM_CHANNEL_4,(uint32_t *)send_Buf,234*24+224);
}
void WS_Write_Rectangle_mid(uint8_t n_R,uint8_t n_G,uint8_t n_B)
{
	WS2812_close_Rectangle();
	HAL_Delay(10);
	for(uint16_t n=0;n<165;n++)
		{
			SetPixelColor(n,n_R,n_G,n_B);
		}
	 HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t *)send_Buf,165*24+224);
}
