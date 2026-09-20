#ifndef ENERGY_H
#define ENERGY_H

#include <stdint.h>

typedef enum {
    ENERGY_MODE_OFF = 0,
    ENERGY_MODE_SMALL_TEST,
    ENERGY_MODE_BIG_TEST
} EnergyMode_t;
typedef enum {
    ENERGY_RESULT_MISS = 0,
    ENERGY_RESULT_HIT
} EnergyResult_t;

typedef struct {
    uint32_t rev;
    uint32_t round;
    EnergyMode_t mode;
    uint8_t phase; /* 0 idle, 1 running, 2 flashing, 3 complete */
    uint8_t mask;
    uint8_t target;
    uint8_t flash_on;
    uint8_t flashes;
    uint8_t progress; /* Big-rune completed bars: 0..5. */
    uint8_t other_target; /* The second active big-rune board, or 0. */
} EnergyState_t;

void Energy_SetMode(EnergyMode_t mode);
void Energy_ReportResult(EnergyResult_t result);
void Energy_Task(void);
EnergyState_t Energy_GetState(void);
uint8_t Energy_IsSettled(void);

#endif
