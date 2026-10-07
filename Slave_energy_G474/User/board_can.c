#include "board_can.h"
#include "fdcan.h"
#include "ws2812_spi.h"
#include "ws2812_spi2.h"
#include "target.h"
#include "main.h"

static volatile BoardCommand_t board_can_received_command = BOARD_CMD_OFF;  
static volatile uint8_t board_can_received_progress = 0U;
static volatile uint8_t board_can_command_pending = 0;  // 是否有新命令待处理
static BoardCommand_t board_can_applied_command = 0xFF;
static uint8_t board_can_applied_progress = 0U;
static uint8_t board_can_node_id = 0U;

static uint8_t BoardCan_ReadNodeId(void) {
    uint8_t id = 0U;
    if (HAL_GPIO_ReadPin(GPIO_ID1_GPIO_Port, GPIO_ID1_Pin) == GPIO_PIN_SET) id |= 1U;
    if (HAL_GPIO_ReadPin(GPIO_ID2_GPIO_Port, GPIO_ID2_Pin) == GPIO_PIN_SET) id |= 2U;
    if (HAL_GPIO_ReadPin(GPIO_ID3_GPIO_Port, GPIO_ID3_Pin) == GPIO_PIN_SET) id |= 4U;
    return id;
}

uint8_t BoardCan_Init(void) {
    board_can_node_id = BoardCan_ReadNodeId();
    if ((board_can_node_id < BOARD_CAN_NODE_MIN) || (board_can_node_id > BOARD_CAN_NODE_MAX)) {
        return 0U;
    }
    FDCAN_FilterTypeDef filter = {0};
    filter.IdType = FDCAN_STANDARD_ID;
    filter.FilterIndex = 0;
    filter.FilterType = FDCAN_FILTER_DUAL;
    filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    filter.FilterID1 = BOARD_CAN_BROADCAST_ID + board_can_node_id;
    filter.FilterID2 = BOARD_CAN_BROADCAST_ID;
    if (HAL_FDCAN_ConfigFilter(&hfdcan3, &filter) != HAL_OK) {
        return 0;
    }
    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan3, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE) != HAL_OK) {
        return 0;
    }
    if (HAL_FDCAN_Start(&hfdcan3) != HAL_OK) {
        return 0;
    }
    if (HAL_FDCAN_ActivateNotification(&hfdcan3, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
        return 0;  // 触发FDCAN3_IT0中断
    }
    return 1;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {  // 那个FDCAN触发回调，FIFO发生什么
    FDCAN_RxHeaderTypeDef rx_header = {0};  // CAN ID 
    uint8_t rx_data[8] = {0};  // 保存8bytes数据
    if (hfdcan->Instance != FDCAN3) {
        return;
    }  // 不是FDCAN3触发的不管
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0) {
        return;
    }
    if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, rx_data) != HAL_OK) {
        return;
    }
    if ((rx_header.IdType != FDCAN_STANDARD_ID) ||
        ((rx_header.Identifier != BOARD_CAN_BROADCAST_ID + board_can_node_id) &&
         (rx_header.Identifier != BOARD_CAN_BROADCAST_ID))) {
        return;
    }  // 只接收本从板专用ID或全体广播ID
    if ((rx_header.DataLength < FDCAN_DLC_BYTES_1) ||
        (rx_data[0] > BOARD_CMD_BIG_FINISH)) {
        return;
    }
    if (rx_data[0] == BOARD_CMD_PROGRESS) {
        if ((rx_header.DataLength < FDCAN_DLC_BYTES_2) ||
            (rx_data[1] < 1U) || (rx_data[1] > 5U)) return;
        board_can_received_progress = rx_data[1];
    }
    board_can_received_command = (BoardCommand_t)rx_data[0];
    board_can_command_pending = 1;
}  
void BoardCan_Task(void) {
    BoardCommand_t command;
    uint8_t progress;
    if (board_can_command_pending == 0) {
        return;
    }
    board_can_command_pending = 0;
    command = board_can_received_command;
    progress = board_can_received_progress;
    if ((command == board_can_applied_command) &&
        ((command != BOARD_CMD_PROGRESS) || (progress == board_can_applied_progress))) {
        return;
    }  // 防止收到相同箭头命令从头开始不连贯)
    //
    if ((WS2812_IsBusy() != 0) || (Perimeter_IsBusy() != 0) ) {
        board_can_command_pending = 1;
        return;
    }
    switch (command) {
        case BOARD_CMD_OFF:
            Arrow_ShowOff();
            Perimeter_ShowOff();
            Target_Show(TARGET_BLACK);
            break;
        case BOARD_CMD_TARGET_ARROW:
            Arrow_StartFlow(50, 0, 0);
            Perimeter_ShowOff();
            Target_Show(TARGET_TARGET);
            break;
        case BOARD_CMD_SMALL_HIT:
            Arrow_ShowAllRed();
            Perimeter_ShowAllRed();
            Target_Show(TARGET_RING_1);
            break;
        case BOARD_CMD_PROGRESS:
            Arrow_ShowProgress(progress);
            Perimeter_ShowProgress(progress);
            Target_Show(TARGET_BLACK);
            break;
        case BOARD_CMD_SMALL_FINISH:
            Arrow_ShowAllRed();
            Perimeter_ShowAllRed();
            Target_Show(TARGET_RING_1);
            break;
        case BOARD_CMD_BIG_FINISH:
            Arrow_ShowAllRed();
            Perimeter_ShowAllRed();
            Target_Show(TARGET_RING_8);
            break;
        default:
            break;
    }
    board_can_applied_command = command;
    if (command == BOARD_CMD_PROGRESS) board_can_applied_progress = progress;
}


