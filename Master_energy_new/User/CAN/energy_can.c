#include "energy_can.h"
#include "can.h"

uint8_t EnergyCan_Init(void) {
    CAN_FilterTypeDef filter = {0};
    filter.FilterBank = 0;  // 过滤器组0
    filter.FilterMode = CAN_FILTERMODE_IDMASK;  // 目标ID+掩码
    filter.FilterScale = CAN_FILTERSCALE_32BIT;  // 过滤器位宽
    filter.FilterIdHigh = ENERGY_CAN_REPLY_ID_BASE << 5;  // STM32的32位CAN过滤器并不是把11位标准ID直接放在最低位，而是规定标准ID放在过滤器高16位的第15～5位
    filter.FilterMaskIdHigh = ENERGY_CAN_REPLY_ID_MASK << 5;
    filter.FilterIdLow = CAN_ID_STD | CAN_RTR_DATA;  // 标准ID+普通数据帧RTR=0
    filter.FilterMaskIdLow = CAN_ID_EXT | CAN_RTR_REMOTE;  // 检查IDE和RTR位，排除扩展帧和远程帧
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;  // 通过过滤器的消息进入CAN接受队列0
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14;  // Filter Bank 0～13  → CAN1; 14～27 → CAN2
    if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK) {
        return 0;
    }
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {  // 启动CAN外设，使其可以发送和接收
        return 0;
    }
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        return 0;
    }  // 启用了CAN外设内部的中断通知
    return 1;
}  // 掩码位为1：对应的ID位必须完全相同；掩码位为0：不检查对应的ID位

uint8_t EnergyCan_SendCommand(uint8_t node_id, EnergyCommand_t command) {
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t tx_mailbox = 0;
    uint8_t tx_data[1] = {0};
    if (node_id == ENERGY_CAN_BROADCAST_NODE) {
        if ((command != ENERGY_CMD_OFF) && (command != ENERGY_CMD_SMALL_FINISH) &&
            (command != ENERGY_CMD_BIG_FINISH)) {
            return 0;
        }
    } else if ((node_id < ENERGY_CAN_NODE_MIN) || (node_id > ENERGY_CAN_NODE_MAX)) {
        return 0;
    }
    if ((command < ENERGY_CMD_OFF) || (command > ENERGY_CMD_BIG_FINISH) ||
        (command == ENERGY_CMD_PROGRESS)) {
        return 0;
    }
    tx_header.StdId = ENERGY_CAN_COMMAND_ID_BASE + node_id;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 1;
    tx_data[0] = command;
    if (HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, &tx_mailbox) != HAL_OK) {
        return 0;
    }
    return 1;
}

uint8_t EnergyCan_SendProgress(uint8_t node_id, uint8_t level) {
    if ((node_id < ENERGY_CAN_NODE_MIN) || (node_id > ENERGY_CAN_NODE_MAX) ||
        (level < 1U) || (level > 5U)) return 0U;
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t tx_mailbox = 0U;
    uint8_t tx_data[2] = {ENERGY_CMD_PROGRESS, level};
    tx_header.StdId = ENERGY_CAN_COMMAND_ID_BASE + node_id;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = 2U;
    return HAL_CAN_AddTxMessage(&hcan1, &tx_header, tx_data, &tx_mailbox) == HAL_OK;
}
