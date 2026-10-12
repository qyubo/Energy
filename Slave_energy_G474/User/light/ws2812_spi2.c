#include "ws2812_spi2.h"
#include "spi.h"
#include <string.h>
#include "ws2812_protocol.h"

#define PERIMETER_TX_BUFFER_SIZE (PERIMETER_LED_COUNT * WS2812_BYTES_PER_PIXEL + WS2812_RESET_BYTES)

static uint8_t perimeter_pixel_color[PERIMETER_LED_COUNT][3];
static uint8_t perimeter_tx_buffer[PERIMETER_TX_BUFFER_SIZE];
static volatile uint8_t perimeter_dma_busy = 0;
static const uint8_t perimeter_progress_start[6] = {0, 6, 5, 3, 1, 1};
static const uint8_t perimeter_progress_end[6] = {0, 10, 11, 13, 15, 15};

void Perimeter_Init(void) {
    perimeter_dma_busy = 0;
    Perimeter_Clear();
}

uint8_t Perimeter_IsBusy(void) {
    return perimeter_dma_busy;
}
void Perimeter_SetPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue) {

    if (index >= PERIMETER_LED_COUNT) {
        return; 
    }

    perimeter_pixel_color[index][0] = green;
    perimeter_pixel_color[index][1] = red;
    perimeter_pixel_color[index][2] = blue;
}
void Perimeter_Fill(uint8_t red, uint8_t green, uint8_t blue) {
    for (uint16_t i = 0; i < PERIMETER_LED_COUNT; i++) {
        Perimeter_SetPixel(i, red, green, blue);
    }
}
void Perimeter_Clear(void) {
    Perimeter_Fill(0, 0, 0);
}

uint8_t Perimeter_ShowAllRed(void) {
    if (Perimeter_IsBusy() != 0) {
        return 0;
    }
    Perimeter_Fill(100, 0, 0);
    return Perimeter_Show();
}
uint8_t Perimeter_ShowOff(void) {
    if (Perimeter_IsBusy() != 0) {
        return 0;
    }
    Perimeter_Clear();
    return Perimeter_Show();
}
uint8_t Perimeter_ShowProgress(uint8_t level) {
    if ((level < 1) || (level > 5)) {
        return 0;
    }
    if (Perimeter_IsBusy() != 0) {
        return 0;
    }
    Perimeter_Clear();
    for (uint8_t index = perimeter_progress_start[level]; index <= perimeter_progress_end[level]; index++) {
        Perimeter_SetPixel(index - 1, 100, 0, 0);
    }

    return Perimeter_Show();
}


void Perimeter_TxCompleteCallback(void) {
    perimeter_dma_busy = 0;
}
void Perimeter_TxErrorCallback(void) {
    perimeter_dma_busy = 0;
}

static void Perimeter_EncodeByte(uint8_t value, uint8_t *output) {
    uint8_t first_code;
    uint8_t second_code;

    for (uint8_t i = 0; i < 4; i++) {
        if ((value & 0x80) != 0) {  //0x80 = 10000000
            first_code = WS2812_CODE_1;
        }
        else {
            first_code = WS2812_CODE_0;
        }
        value <<= 1;

        if ((value & 0x80) != 0) {  
            second_code = WS2812_CODE_1;
        }
        else {
            second_code = WS2812_CODE_0;
        }

        output[i] = (uint8_t)((first_code << 4) | second_code);
        value <<= 1;
    }
}

uint8_t Perimeter_Show(void) {  // 发送数据到WS2812
    uint16_t output_index = 0;
    if (perimeter_dma_busy == 1) {
        return 0;
    }

    memset(perimeter_tx_buffer, 0, sizeof(perimeter_tx_buffer));  // memory set-memset(目标地址, 要填入的值, 字节数量);

    for (uint16_t led = 0; led < PERIMETER_LED_COUNT; led++) {
        for (uint8_t i = 0; i < 3; i++) {  // i=0,green; i=1,red; i=2,blue
            Perimeter_EncodeByte(perimeter_pixel_color[led][i], &perimeter_tx_buffer[output_index]);
            output_index += 4;
        }
    }

    perimeter_dma_busy = 1;

    if (HAL_SPI_Transmit_DMA(&hspi2, perimeter_tx_buffer, sizeof(perimeter_tx_buffer)) != HAL_OK) {
        perimeter_dma_busy = 0;
        return 0;
    }

    return 1;
}