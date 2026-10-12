#include "ws2812_spi.h"
#include "spi.h"
#include <string.h>
#include "ws2812_protocol.h"
#include "ws2812_spi2.h"

#define WS2812_TX_BUFFER_SIZE (WS2812_LED_COUNT * WS2812_BYTES_PER_PIXEL + WS2812_RESET_BYTES)
#define ARROW_ROWS 5
#define ARROW_COLS 33
#define ARROW_PATTERN_PERIOD 4  // 箭头重复周期
#define ARROW_LIT_WIDTH 2  // 箭头点亮宽度是2颗灯珠
#define ARROW_FRAME_MS 50  // 每隔50ms切换一帧
/* Five nearly equal left-to-right sections: 7, 6, 7, 6, 7 columns. */
static const uint8_t arrow_progress_end_col[6] = {0, 7, 13, 20, 26, 33};

static uint8_t pixel_color[WS2812_LED_COUNT][3];  // Array to hold RGB values for each LED
static uint8_t tx_buffer[WS2812_TX_BUFFER_SIZE];
static volatile uint8_t dma_busy = 0;
static uint8_t arrow_flow_running = 0;
static uint8_t arrow_phase = 0;
static uint8_t arrow_red = 0;
static uint8_t arrow_green = 0;
static uint8_t arrow_blue = 0;
static uint32_t arrow_last_tick = 0;
// left -> right
// left <- right
static uint16_t Arrow_XYToIndex(uint8_t row, uint8_t col) {
    if ((row & 1) == 0) {  // even
        return row * ARROW_COLS + col;
    }
    return row * ARROW_COLS + (ARROW_COLS -1 - col);  // odd
}
static void Arrow_SetPixel(uint8_t row, uint8_t col, uint8_t red, uint8_t green, uint8_t blue) {
    if ((row >= ARROW_ROWS) || (col >= ARROW_COLS)) {
        return;
    }
    uint16_t index = Arrow_XYToIndex(row, col);
    WS2812_SetPixel(index, red, green, blue);
}
static void Arrow_SetColumn(uint8_t col, uint8_t red, uint8_t green, uint8_t blue) {
    if (col >= ARROW_COLS) {
        return;
    }
    for (uint8_t row = 0; row < ARROW_ROWS; row++) {
        Arrow_SetPixel(row, col, red, green, blue);
    }
} // 设置一整列颜色
static uint8_t Arrow_GetRowOffset(uint8_t row) {  
    uint8_t center_row = ARROW_ROWS / 2;
    if (row <= center_row) {
        return row;
    }
    return ARROW_ROWS - 1 - row;
}  // 获取行偏移量
static void Arrow_DrawFlowFrame(uint8_t phase, uint8_t red, uint8_t green, uint8_t blue) {  //流动动画帧
    WS2812_Clear();
    for (uint8_t row = 0; row < ARROW_ROWS; row++) {
        uint8_t start_col = (Arrow_GetRowOffset(row) + phase) % ARROW_PATTERN_PERIOD;
        for (uint8_t col = start_col; col < ARROW_COLS; col += ARROW_PATTERN_PERIOD) {
            for (uint8_t width = 0; width < ARROW_LIT_WIDTH; width++) {
                Arrow_SetPixel(row, col + width, red, green, blue);
            }
        }
    }
}  // 生成一帧箭头  
void Arrow_StartFlow(uint8_t red, uint8_t green, uint8_t blue) {
    arrow_red = red;
    arrow_green = green;
    arrow_blue = blue;
    arrow_phase = 0;
    arrow_flow_running = 1;
    arrow_last_tick = HAL_GetTick();
}  // 启动流水状态
void Arrow_StopFlow(void) {
    arrow_flow_running = 0;
    arrow_phase = 0;
}  // 停
uint8_t Arrow_ShowAllRed(void) {
    if (WS2812_IsBusy() != 0) {
        return 0;
    }
    Arrow_StopFlow();
    WS2812_Fill(50, 0, 0);
    return WS2812_Show();
}  // 全红
uint8_t Arrow_ShowProgress(uint8_t level) {
    if ((level < 1U) || (level > 5U) || (WS2812_IsBusy() != 0U)) {
        return 0;
    }
    Arrow_StopFlow();
    WS2812_Clear();
    for (uint8_t col = 0; col < arrow_progress_end_col[level]; col++) {
        Arrow_SetColumn(col, 50, 0, 0);
    }
    return WS2812_Show();
}  // Progress grows from physical column 0 to column 32, same as the arrow flow.
uint8_t Arrow_ShowOff(void) {
    if (WS2812_IsBusy() != 0) {
        return 0;
    }
    Arrow_StopFlow();
    WS2812_Clear();
    return WS2812_Show();
}
void Arrow_Task(void) {
    uint32_t now = HAL_GetTick();
    if (arrow_flow_running == 0 || WS2812_IsBusy() != 0 || now - arrow_last_tick < ARROW_FRAME_MS) {
        return;
    }
    Arrow_DrawFlowFrame(arrow_phase, arrow_red, arrow_green, arrow_blue);
    if (WS2812_Show() != 0) {
        arrow_phase = (arrow_phase +1) % ARROW_PATTERN_PERIOD;
        arrow_last_tick = now;
    }
}  // 流动箭头

void WS2812_Init(void) {
    dma_busy = 0;
    WS2812_Clear();
}

uint8_t WS2812_IsBusy(void) {
    return dma_busy;
}

void WS2812_SetPixel(uint16_t index, uint8_t red, uint8_t green, uint8_t blue) {

    if (index >= WS2812_LED_COUNT) {
        return; 
    }

    pixel_color[index][0] = green;
    pixel_color[index][1] = red;
    pixel_color[index][2] = blue;
}

// fill all LEDs with the same color
void WS2812_Fill(uint8_t red, uint8_t green, uint8_t blue) {
    for (uint16_t i = 0; i < WS2812_LED_COUNT; i++) {
        WS2812_SetPixel(i, red, green, blue);
    }
}

// set all LEDs to off
void WS2812_Clear(void) {
    WS2812_Fill(0, 0, 0);
}

static void WS2812_EncodeByte(uint8_t value, uint8_t *output) {
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

uint8_t WS2812_Show(void) {  // 发送数据到WS2812
    uint16_t output_index = 0;
    if (dma_busy == 1) {
        return 0;
    }

    memset(tx_buffer, 0, sizeof(tx_buffer));  // memory set-memset(目标地址, 要填入的值, 字节数量);

    for (uint16_t led = 0; led < WS2812_LED_COUNT; led++) {
        for (uint8_t i = 0; i < 3; i++) {  // i=0,green; i=1,red; i=2,blue
            WS2812_EncodeByte(pixel_color[led][i], &tx_buffer[output_index]);
            output_index += 4;
        }
    }

    dma_busy = 1;

    if (HAL_SPI_Transmit_DMA(&hspi1, tx_buffer, sizeof(tx_buffer)) != HAL_OK) {
        dma_busy = 0;
        return 0;
    }

    return 1;
}

// DMA传输完成回调
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {  // DMA每向SPI1传送1字节，计数器减1；2108字节全部传完且SPI发送结束后，HAL调用本函数。
    if (hspi->Instance == SPI1) {  // (*hspi).Instance
        dma_busy = 0;
    }
    else if (hspi->Instance == SPI2) {
        Perimeter_TxCompleteCallback();
    }
}
// DMA传输错误回调
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        dma_busy = 0;
    }
    else if (hspi->Instance == SPI2) {
        Perimeter_TxErrorCallback();
    }
}
