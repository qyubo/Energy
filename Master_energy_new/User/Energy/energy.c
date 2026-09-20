#include "energy.h"
#include "energy_can.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

#define ENERGY_ALL_DONE_MASK 0x1F
#define ENERGY_FLASH_COUNT 10
#define ENERGY_FLASH_HALF_PERIOD_MS 250

typedef enum {
    BIG_NEED_PAIR = 0,
    BIG_PAIR,
    BIG_SINGLE,
    BIG_FINAL,
    BIG_FLASH
} BigPhase_t;

static EnergyMode_t energy_mode = ENERGY_MODE_OFF;
static uint8_t completed_mask = 0;
static uint8_t current_target = 0;
static uint8_t result_pending = 0;
static EnergyResult_t pending_result = ENERGY_RESULT_MISS;
static uint8_t reset_pending = 1U;  // 需要发送全体关灯广播
static uint32_t random_state = 0xA341316C;
static uint8_t finish_pending = 1U;
static uint8_t flash_step = 0U;
static uint32_t flash_last_ms = 0U;
static uint32_t energy_rev = 0U;
static uint32_t energy_round = 0U;
static BigPhase_t big_phase = BIG_NEED_PAIR;
static uint8_t big_progress = 0U;
static uint8_t big_other_target = 0U;
static uint8_t big_output_next_node = 0U;


static uint8_t Energy_SelectNextTarget(void) {
    uint8_t available[ENERGY_CAN_NODE_MAX];
    uint8_t count = 0;
    for (uint8_t node = ENERGY_CAN_NODE_MIN; node <= ENERGY_CAN_NODE_MAX; node++) {
        if ((completed_mask & (1 << (node-1))) == 0) {
            available[count++] = node;
        }
    }
    if (count == 0) {
        return 0; 
    }
    random_state ^= HAL_GetTick();
    random_state = random_state * 1664525 + 1013904223;
    return available[(random_state >> 16) % count];
}

static uint8_t Energy_RandomNodeExcept(uint8_t excluded) {
    uint8_t nodes[ENERGY_CAN_NODE_MAX];
    uint8_t count = 0U;
    for (uint8_t node = ENERGY_CAN_NODE_MIN; node <= ENERGY_CAN_NODE_MAX; node++) {
        if (node != excluded) nodes[count++] = node;
    }
    random_state ^= HAL_GetTick();
    random_state = random_state * 1664525U + 1013904223U;
    return nodes[(random_state >> 16) % count];
}

/* Queue one frame per pass; retry when all CAN TX mailboxes are busy. */
static uint8_t Energy_BigSendNode(uint8_t node) {
    if (big_phase == BIG_FINAL) {
        if (node == current_target)
            return EnergyCan_SendCommand(node, ENERGY_CMD_FINAL_TARGET);
        return EnergyCan_SendProgress(node, 5U);
    }
    if ((node == current_target) ||
        ((big_phase == BIG_PAIR) && (node == big_other_target)))
        return EnergyCan_SendCommand(node, ENERGY_CMD_TARGET_ARROW);
    if (big_progress == 0U)
        return EnergyCan_SendCommand(node, ENERGY_CMD_OFF);
    return EnergyCan_SendProgress(node, big_progress);
}

static void Energy_BigNewPair(uint8_t restart) {
    big_progress = 0U;
    current_target = Energy_RandomNodeExcept(0U);
    big_other_target = Energy_RandomNodeExcept(current_target);
    big_phase = BIG_PAIR;
    big_output_next_node = ENERGY_CAN_NODE_MIN;
    result_pending = 0U;
    if (restart != 0U) {
        energy_round++;
        reset_pending = 1U; /* Clear the old pair before lighting the new one. */
    }
    energy_rev++;
}

static void Energy_FlashTask(EnergyCommand_t finish_command) {
    if (finish_pending != 0U) {
        if (EnergyCan_SendCommand(ENERGY_CAN_BROADCAST_NODE, finish_command) != 0U) {
            finish_pending = 0U;
            flash_last_ms = HAL_GetTick();
            energy_rev++;
        }
        return;
    }
    if ((flash_step < 2U * ENERGY_FLASH_COUNT) &&
        ((uint32_t)(HAL_GetTick() - flash_last_ms) >= ENERGY_FLASH_HALF_PERIOD_MS)) {
        EnergyCommand_t command = ((flash_step & 1U) == 0U) ? ENERGY_CMD_OFF : finish_command;
        if (EnergyCan_SendCommand(ENERGY_CAN_BROADCAST_NODE, command) != 0U) {
            flash_step++;
            flash_last_ms = HAL_GetTick();
            energy_rev++;
        }
    }
}

void Energy_SetMode(EnergyMode_t mode) {
    if ((mode < ENERGY_MODE_OFF) || (mode > ENERGY_MODE_BIG_TEST)) {
        return;
    }
    energy_mode = mode;
    energy_rev++;
    if (mode != ENERGY_MODE_OFF) energy_round++;
    completed_mask = 0;
    current_target = 0;
    result_pending = 0;
    pending_result = ENERGY_RESULT_MISS;
    reset_pending = 1U;
    finish_pending = 1U;
    flash_step = 0U;
    flash_last_ms = 0U;
    big_phase = BIG_NEED_PAIR;
    big_progress = 0U;
    big_other_target = 0U;
    big_output_next_node = 0U;
}
void Energy_ReportResult(EnergyResult_t result) {
    if ((result != ENERGY_RESULT_HIT) && (result != ENERGY_RESULT_MISS)) {
        return;
    }
    if (current_target == 0) {
        return;
    }
    pending_result = result;
    result_pending = 1;
}
 
void Energy_Task(void) {
    if (reset_pending != 0U) {
        if (EnergyCan_SendCommand(ENERGY_CAN_BROADCAST_NODE, ENERGY_CMD_OFF) != 0U) {
            reset_pending = 0U;
        }
        return;
    }
    switch (energy_mode) {
        case ENERGY_MODE_OFF:
            break;
        case ENERGY_MODE_SMALL_TEST:
            if ((result_pending != 0) && (current_target != 0)) {
                if (pending_result == ENERGY_RESULT_HIT) {
                    if (EnergyCan_SendCommand(current_target, ENERGY_CMD_SMALL_HIT) != 0) {
                        completed_mask |= (1 << (current_target - 1));
                        current_target = 0;
                        result_pending = 0;
                        energy_rev++;
                    }
                }
                else {
                    completed_mask = 0;
                    current_target = 0;
                    result_pending = 0;
                    reset_pending = 1U;
                    energy_round++;
                    energy_rev++;
                }
            }
            if (completed_mask == ENERGY_ALL_DONE_MASK) {
                Energy_FlashTask(ENERGY_CMD_FINISH);
                break;
            }
            if ((current_target == 0) && (result_pending == 0) && (reset_pending == 0U) && (completed_mask != ENERGY_ALL_DONE_MASK)) {
                uint8_t next_target = Energy_SelectNextTarget();
                if ((next_target != 0) && (EnergyCan_SendCommand(next_target, ENERGY_CMD_TARGET_ARROW) != 0)) {
                    current_target = next_target;
                    energy_rev++;
                }
            }
            break;
        case ENERGY_MODE_BIG_TEST:
            if (big_phase == BIG_FLASH) {
                Energy_FlashTask(ENERGY_CMD_BIG_FINISH);
                break;
            }
            if (big_phase == BIG_NEED_PAIR) Energy_BigNewPair(0U);
            if (big_output_next_node != 0U) {
                if (Energy_BigSendNode(big_output_next_node) != 0U) {
                    big_output_next_node++;
                    if (big_output_next_node > ENERGY_CAN_NODE_MAX) {
                        big_output_next_node = 0U;
                    }
                }
                break;
            }
            if (big_phase == BIG_PAIR) {
                if ((result_pending != 0U) && (pending_result == ENERGY_RESULT_HIT)) {
                    result_pending = 0U;
                    current_target = big_other_target; /* The unhit board stays active. */
                    big_other_target = 0U;
                    if (big_progress == 4U) {
                        big_progress = 5U;
                        big_phase = BIG_FINAL;
                    } else {
                        big_phase = BIG_SINGLE;
                    }
                    big_output_next_node = ENERGY_CAN_NODE_MIN;
                    energy_rev++;
                } else if (result_pending != 0U) {
                    /* A two-target miss restarts at zero progress. */
                    Energy_BigNewPair(1U);
                }
            } else if (big_phase == BIG_SINGLE) {
                if (result_pending != 0U) {
                    /* Single-target hit or miss advances to a new pair. */
                    result_pending = 0U;
                    big_progress++;
                    big_other_target = Energy_RandomNodeExcept(current_target);
                    big_phase = BIG_PAIR;
                    big_output_next_node = ENERGY_CAN_NODE_MIN;
                    energy_rev++;
                }
            } else if ((big_phase == BIG_FINAL) && (result_pending != 0U)) {
                if (pending_result == ENERGY_RESULT_HIT) {
                    result_pending = 0U;
                    current_target = 0U;
                    completed_mask = ENERGY_ALL_DONE_MASK;
                    big_phase = BIG_FLASH;
                    energy_rev++;
                } else {
                    Energy_BigNewPair(1U); /* Final-target miss restarts. */
                }
            }
            break;
        default:
            break;
    }

}

uint8_t Energy_IsSettled(void) {
    if (energy_mode == ENERGY_MODE_BIG_TEST) {
        return (reset_pending == 0U) && (result_pending == 0U) &&
               (big_output_next_node == 0U) &&
               ((big_phase == BIG_FLASH) ? (finish_pending == 0U) : (current_target != 0U));
    }
    return (reset_pending == 0U) && (result_pending == 0U) &&
           ((energy_mode == ENERGY_MODE_OFF) || (current_target != 0U) ||
            ((completed_mask == ENERGY_ALL_DONE_MASK) && (finish_pending == 0U)));
}

EnergyState_t Energy_GetState(void) {
    EnergyState_t state = {0};
    state.rev = energy_rev;
    state.round = energy_round;
    state.mode = energy_mode;
    state.mask = completed_mask;
    state.target = current_target;
    if (energy_mode == ENERGY_MODE_BIG_TEST) {
        state.progress = big_progress;
        state.other_target = (big_phase == BIG_PAIR) ? big_other_target : 0U;
    }
    if (energy_mode == ENERGY_MODE_OFF) {
        state.phase = 0U;
    } else if (completed_mask == ENERGY_ALL_DONE_MASK) {
        state.phase = (flash_step == 2U * ENERGY_FLASH_COUNT) ? 3U : 2U;
        state.flash_on = (finish_pending != 0U) || ((flash_step & 1U) == 0U);
        state.flashes = flash_step / 2U;
    } else {
        state.phase = 1U;
    }
    return state;
}
