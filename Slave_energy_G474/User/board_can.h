#ifndef BOARD_CAN_H
#define BOARD_CAN_H

#include <stdint.h>

#define BOARD_CAN_BROADCAST_ID 0x300U
#define BOARD_CAN_NODE_MIN 1U
#define BOARD_CAN_NODE_MAX 5U

typedef enum {
    BOARD_CMD_OFF = 0,  // Arrow&Target off
    BOARD_CMD_TARGET_ARROW,  // Arrow&Target on
    BOARD_CMD_SMALL_HIT,  // AllRed, Ring1
    BOARD_CMD_PROGRESS,  // Byte 1: progress level 1..5; target black
    BOARD_CMD_FINISH,  // Small-rune finish: AllRed, Ring1
    BOARD_CMD_BIG_FINISH,  // Big-rune finish: AllRed, Ring8
    BOARD_CMD_FINAL_TARGET,  // Big-rune level 5: AllRed, target still hittable
} BoardCommand_t;

uint8_t BoardCan_Init(void);

void BoardCan_Task(void);



#endif
