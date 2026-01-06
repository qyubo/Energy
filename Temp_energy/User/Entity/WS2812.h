#ifndef _WS2812_H
#define _WS2812_H

#include "main.h"

#define PIXEL_NUM_FRAME  77
#define NUM_FRAME (24*PIXEL_NUM_FRAME + 224)        // Reset 280us / 1.25us = 224,1.25us??

#define PIXEL_NUM_REC 300
#define NUM_REC (24*PIXEL_NUM_REC + 224)        // Reset 280us / 1.25us = 224

#define PIXEL_NUM_FRAME_UP  88
#define NUM_FRAME_UP (24*PIXEL_NUM_FRAME_UP + 224)        // Reset 280us / 1.25us = 224

#define PIXEL_NUM_FRAME_DOWN  92
#define NUM_FRAME_DOWN (24*PIXEL_NUM_FRAME_DOWN + 224)        // Reset 280us / 1.25us = 224

#define PIXEL_NUM_CIRCLE  275
#define NUM_CIRCLE (24*PIXEL_NUM_CIRCLE + 224)        // Reset 280us / 1.25us = 224

#define PIXEL_TEN_RING_NUM  4
#define TEN_RING_NUM (24*PIXEL_TEN_RING_NUM + 224) 

#define PIXEL_NINE_RING_NUM  8
#define NINE_RING_NUM (24*PIXEL_NINE_RING_NUM + 224)

#define PIXEL_EIGHT_RING_NUM  12
#define EIGHT_RING_NUM (24*PIXEL_EIGHT_RING_NUM + 224)

#define PIXEL_SEVEN_RING_NUM  16
#define SEVEN_RING_NUM (24*PIXEL_SEVEN_RING_NUM + 224)

#define PIXEL_SIX_RING_NUM  20
#define SIX_RING_NUM (24*PIXEL_SIX_RING_NUM + 224)

#define PIXEL_FIVE_RING_NUM  24
#define FIVE_RING_NUM (24*PIXEL_FIVE_RING_NUM + 224)

#define PIXEL_FOUR_RING_NUM  28
#define FOUR_RING_NUM (24*PIXEL_FOUR_RING_NUM + 224)

#define PIXEL_THREE_RING_NUM  32
#define THREE_RING_NUM (24*PIXEL_THREE_RING_NUM + 224)

#define PIXEL_TWO_RING_NUM  36
#define TWO_RING_NUM (24*PIXEL_TWO_RING_NUM + 224)

#define PIXEL_ONE_RING_NUM  54
#define ONE_RING_NUM (24*PIXEL_ONE_RING_NUM + 224)

#define WS1 33  //?????????? 
#define WS0 12



void WS2812_close_all();
void SetPixelColor(uint16_t n,uint8_t red,uint8_t green,uint8_t blue);
uint32_t WS2812_color(uint8_t red,uint8_t green,uint8_t blue);
void WS2812_close_Rectangle();
void WS_Write_RGB_Frame_up(uint8_t n_R, uint8_t n_G, uint8_t n_B);
void WS_Write_RGB_Frame_down(uint8_t n_R, uint8_t n_G, uint8_t n_B);
void WS_Write_RGB_TEN_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_RGB_NINE_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_RGB_EIGHT_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_RGB_SEVEN_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_RGB_SIX_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_RGB_FIVE_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_RGB_FOUR_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_RGB_THREE_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_RGB_TWO_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_RGB_ONE_RINGS(uint8_t n_R, uint8_t n_G, uint8_t n_B );
void WS_Write_arrow_flow_LED(uint8_t n_R, uint8_t n_G, uint8_t n_B);
void WS_Write_Rectangle_up(uint8_t n_R,uint8_t n_G,uint8_t n_B);
void WS2812_Write_all();
void WS_Write_Cross(uint8_t n_R,uint8_t n_G,uint8_t n_B);
void WS2812_close_Frame_up();
void WS2812_close_Frame_down();
void WS_Write_Rectangle_down(uint8_t n_R,uint8_t n_G,uint8_t n_B);
void WS_Write_Rectangle_mid(uint8_t n_R,uint8_t n_G,uint8_t n_B);
#endif
