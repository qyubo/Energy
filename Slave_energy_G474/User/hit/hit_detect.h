#ifndef HIT_DETECT_H
#define HIT_DETECT_H

#include "hit_adc.h"

extern uint16_t hit_baseline[CHANNEL_COUNT];
extern uint16_t hit_block_peak[CHANNEL_COUNT];

void HitDetect_Process(const uint16_t block[FRAMES_PER_BLOCK][CHANNEL_COUNT], uint8_t continuous);
uint8_t HitDetect_IsReady(void);

extern uint16_t hit_event_peak[CHANNEL_COUNT];
uint8_t HitDetect_GetResult(uint8_t *ring);

#endif