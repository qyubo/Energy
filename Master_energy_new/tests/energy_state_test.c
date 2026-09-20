#include "energy.h"
#include "energy_can.h"
#include <assert.h>
#include <stdio.h>

static uint32_t now_ms;
static uint8_t last_command[6];
static uint8_t last_level[6];

uint32_t HAL_GetTick(void) { return now_ms; }
uint8_t EnergyCan_SendCommand(uint8_t node, EnergyCommand_t command) {
    if (node == ENERGY_CAN_BROADCAST_NODE) {
        for (uint8_t i = 1U; i <= 5U; i++) last_command[i] = command;
    } else last_command[node] = command;
    return 1U;
}
uint8_t EnergyCan_SendProgress(uint8_t node, uint8_t level) {
    last_command[node] = ENERGY_CMD_PROGRESS;
    last_level[node] = level;
    return 1U;
}

static void settle(void) {
    for (unsigned i = 0; i < 30U; i++) {
        Energy_Task();
        if (Energy_IsSettled()) return;
        now_ms += 10U;
    }
    assert(!"state did not settle");
}

static void result(EnergyResult_t r) {
    Energy_ReportResult(r);
    settle();
}

static void test_big(void) {
    Energy_SetMode(ENERGY_MODE_BIG_TEST);
    settle();
    EnergyState_t s = Energy_GetState();
    assert(s.mode == ENERGY_MODE_BIG_TEST && s.phase == 1U);
    assert(s.target >= 1U && s.target <= 5U && s.other_target >= 1U &&
           s.other_target <= 5U && s.target != s.other_target && s.progress == 0U);
    uint32_t round = s.round;

    result(ENERGY_RESULT_MISS); /* Pair miss restarts. */
    s = Energy_GetState();
    assert(s.round == round + 1U && s.progress == 0U && s.other_target != 0U);

    result(ENERGY_RESULT_HIT); /* Initial pair -> one remaining board. */
    s = Energy_GetState();
    assert(s.progress == 0U && s.other_target == 0U && s.target != 0U);
    uint8_t retained = s.target;
    result(ENERGY_RESULT_MISS); /* Single miss does not restart. */
    s = Energy_GetState();
    assert(s.round == round + 1U && s.progress == 1U &&
           s.target == retained && s.other_target != retained);
    for (uint8_t i = 1U; i <= 5U; i++) {
        if (i != s.target && i != s.other_target)
            assert(last_command[i] == ENERGY_CMD_PROGRESS && last_level[i] == 1U);
    }

    for (uint8_t level = 1U; level <= 4U; level++) {
        result(ENERGY_RESULT_HIT); /* Required hit from the pair. */
        s = Energy_GetState();
        if (level == 4U) break;
        assert(s.progress == level && s.other_target == 0U);
        result(ENERGY_RESULT_HIT); /* Optional single hit advances too. */
        s = Energy_GetState();
        assert(s.progress == level + 1U && s.other_target != 0U);
    }
    assert(s.progress == 5U && s.other_target == 0U && s.target != 0U);
    assert(last_command[s.target] == ENERGY_CMD_FINAL_TARGET);
    for (uint8_t i = 1U; i <= 5U; i++) {
        if (i != s.target) assert(last_command[i] == ENERGY_CMD_PROGRESS && last_level[i] == 5U);
    }
    round = s.round;
    result(ENERGY_RESULT_MISS); /* Final miss restarts by user choice. */
    s = Energy_GetState();
    assert(s.round == round + 1U && s.progress == 0U && s.other_target != 0U);

    result(ENERGY_RESULT_HIT);
    result(ENERGY_RESULT_MISS);
    for (uint8_t level = 1U; level <= 4U; level++) {
        result(ENERGY_RESULT_HIT);
        if (level < 4U) result(ENERGY_RESULT_MISS);
    }
    assert(Energy_GetState().progress == 5U);
    result(ENERGY_RESULT_HIT);
    assert(Energy_GetState().phase == 2U && Energy_GetState().mask == 31U);
    for (uint8_t i = 1U; i <= 5U; i++) assert(last_command[i] == ENERGY_CMD_BIG_FINISH);
    for (uint8_t step = 0U; step < 20U; step++) {
        now_ms += 250U;
        Energy_Task();
    }
    s = Energy_GetState();
    assert(s.phase == 3U && s.flashes == 10U && s.flash_on != 0U);
    for (uint8_t i = 1U; i <= 5U; i++) assert(last_command[i] == ENERGY_CMD_BIG_FINISH);
}

static void test_small(void) {
    Energy_SetMode(ENERGY_MODE_SMALL_TEST);
    settle();
    for (uint8_t i = 0U; i < 5U; i++) result(ENERGY_RESULT_HIT);
    assert(Energy_GetState().phase == 2U && Energy_GetState().mask == 31U);
    for (uint8_t i = 1U; i <= 5U; i++) assert(last_command[i] == ENERGY_CMD_FINISH);
}

static void test_big_waits_for_input(void) {
    Energy_SetMode(ENERGY_MODE_BIG_TEST);
    settle();
    uint32_t round = Energy_GetState().round;
    uint8_t target = Energy_GetState().target;
    uint8_t other_target = Energy_GetState().other_target;
    now_ms += 10000U;
    settle();
    assert(Energy_GetState().round == round);
    assert(Energy_GetState().progress == 0U);
    assert(Energy_GetState().target == target);
    assert(Energy_GetState().other_target == other_target);
    result(ENERGY_RESULT_HIT);
    assert(Energy_GetState().other_target == 0U);
    target = Energy_GetState().target;
    now_ms += 10000U;
    settle();
    assert(Energy_GetState().progress == 0U);
    assert(Energy_GetState().target == target);
    assert(Energy_GetState().other_target == 0U);
    result(ENERGY_RESULT_MISS);
    assert(Energy_GetState().progress == 1U && Energy_GetState().other_target != 0U);
}

int main(void) {
    test_big();
    test_small();
    test_big_waits_for_input();
    puts("energy state tests passed");
    return 0;
}
