#ifndef WS2812_SPI_H
#define WS2812_SPI_H

#include <stdint.h>  

#define WS2812_LED_COUNT 165  // 5*33

void WS2812_Init(void);  
void WS2812_SetPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
void WS2812_Fill(uint8_t red, uint8_t green, uint8_t blue);
void WS2812_Clear(void);
void Arrow_StartFlow(uint8_t red, uint8_t green, uint8_t blue);
void Arrow_Task(void);  // 更新流水箭头，主循环调用
void Arrow_StopFlow(void);

uint8_t WS2812_IsBusy(void);
uint8_t WS2812_Show(void);
uint8_t Arrow_ShowAllRed(void);
uint8_t Arrow_ShowProgress(uint8_t level);
uint8_t Arrow_ShowOff(void);
#endif



