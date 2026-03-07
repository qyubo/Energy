#include "WS2812.h"
#include <string.h>
uint16_t send_Buf[220 *24+224]={0};
//uint16_t send_Buf_rectangle[50*24+224]={0};
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

 uint32_t WS2812_color(uint8_t red,uint8_t green,uint8_t blue)
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
	for(uint16_t n=0;n<300;n++)
	 {
		 SetPixelColor(n,0,0,0);
	 }
	 HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t *)send_Buf,300*24+224);
}

void WS_Write_Rectangle_up(uint8_t n_R,uint8_t n_G,uint8_t n_B)
{
	memset(send_Buf,0,sizeof(send_Buf));
	for(uint16_t n=0;n<234;n++)
		{
			SetPixelColor(n,n_R,n_G,n_B);
		}
	 HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t *)send_Buf,234*24+224);
}

void WS_Write_Cross(uint8_t n_R,uint8_t n_G,uint8_t n_B)
{
	WS2812_close_all();
	//电源对测
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
		}//此乃最内层
	for(uint16_t n=40;n<61;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
	for(uint16_t n=144;n<180;n++)
		{
		  SetPixelColor(n,n_R,n_G,n_B);
		}
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
//  memset(send_Buf,0,sizeof(send_Buf));
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
	HAL_TIM_PWM_Start_DMA(&htim1,TIM_CHANNEL_1,(uint32_t *)send_Buf,234*24+224);
	HAL_Delay(50);
//	memset(send_Buf,0,sizeof(send_Buf));
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
//	memset(send_Buf,0,sizeof(send_Buf));
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
//	memset(send_Buf,0,sizeof(send_Buf));
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

