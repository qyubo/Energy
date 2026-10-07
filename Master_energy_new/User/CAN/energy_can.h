#ifndef ENERGY_CAN_H
#define ENERGY_CAN_H

#include <stdint.h>

#define ENERGY_CAN_COMMAND_ID_BASE 0x300
#define ENERGY_CAN_BROADCAST_NODE 0
#define ENERGY_CAN_REPLY_ID_BASE 0x100  // 001 0000 0000
#define ENERGY_CAN_REPLY_ID_MASK 0x7F8  // 111 1111 1000  0x100~0x107
#define ENERGY_CAN_NODE_MIN 1
#define ENERGY_CAN_NODE_MAX 5

typedef enum {
    ENERGY_CMD_OFF = 0,
    ENERGY_CMD_TARGET_ARROW,
    ENERGY_CMD_SMALL_HIT,
    ENERGY_CMD_PROGRESS,
    ENERGY_CMD_SMALL_FINISH,
    ENERGY_CMD_BIG_FINISH
} EnergyCommand_t;

uint8_t EnergyCan_Init(void);
uint8_t EnergyCan_SendCommand(uint8_t node_id, EnergyCommand_t command);
uint8_t EnergyCan_SendProgress(uint8_t node_id, uint8_t level);

#endif
