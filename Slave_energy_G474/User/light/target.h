#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>

typedef enum {
    TARGET_BLACK = 0x10,
    TARGET_TARGET = 0x11,
    TARGET_RING_1 = 0x12,
    TARGET_RING_8 = 0x13
} TargetMode_t;

uint8_t Target_Show(TargetMode_t mode);

#endif