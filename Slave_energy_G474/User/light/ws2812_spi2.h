#ifndef WS2812_SPI2_H
#define WS2812_SPI2_H

#include <stdint.h>

#define PERIMETER_LED_COUNT 15

void Perimeter_Init(void);
void Perimeter_SetPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue);
void Perimeter_Fill(uint8_t red, uint8_t green, uint8_t blue);
void Perimeter_Clear(void);
uint8_t Perimeter_Show(void);
uint8_t Perimeter_IsBusy(void);

void Perimeter_TxCompleteCallback(void);
void Perimeter_TxErrorCallback(void);

uint8_t Perimeter_ShowAllRed(void);
uint8_t Perimeter_ShowOff(void);
uint8_t Perimeter_ShowProgress(uint8_t level);

#endif
