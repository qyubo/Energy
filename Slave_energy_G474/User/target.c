#include "target.h"
#include "usart.h"

#define TARGET_FRAME_LENGTH 33
#define TARGET_MODE_BYTE_INDEX 28
#define TARGET_UART_TIMEOUT_MS 20

static uint8_t target_frame[TARGET_FRAME_LENGTH] = {
    0x55U, 0xAAU, 0x00U, 0x00U, 0x01U, 0x01U, 0x00U, 0xC1U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x09U, 0x00U,
    0x00U, 0x00U, 0x09U, 0x00U, 0x00U, 0x04U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x10U, 0x00U, 0x00U, 0x0DU,0x0AU
};
static uint8_t Target_IsModeValid(TargetMode_t mode) {
    switch (mode) {
        case TARGET_BLACK:
        case TARGET_TARGET:
        case TARGET_RING_1:
        case TARGET_RING_8:
            return 1;
        default:
            return 0;
    }
}

uint8_t Target_Show(TargetMode_t mode) {
    if (Target_IsModeValid(mode) == 0) {
        return 0;
    }
    target_frame[TARGET_MODE_BYTE_INDEX] = mode;
    if (HAL_UART_Transmit(&huart2, target_frame, TARGET_FRAME_LENGTH, TARGET_UART_TIMEOUT_MS) != HAL_OK) {
        return 0;
    }
    return 1;
}