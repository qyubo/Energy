//
// Created by cosmosmount on 2025/9/10.
//

#include "main.h"
#include "tx_api.h"

#include "om.h"
#include "magicmsgs.hpp"

TX_THREAD GimbalThread;
uint8_t GimbalThreadStack[4096] = {0};

msg_remoter_t debug_dr16;
msg_ins_t debug_ins;

[[noreturn]] void GimbalThreadFun(ULONG initial_input) {
    UNUSED(initial_input);

    om_suber_t *remoter_suber = om_subscribe(om_find_topic("remoter", UINT32_MAX));
    om_suber_t *ins_suber = om_subscribe(om_find_topic("ins", UINT32_MAX));
    msg_remoter_t remoter{};
    msg_ins_t ins{};


    for (;;) {
        om_suber_export(remoter_suber, &remoter, false);
        om_suber_export(ins_suber, &ins, false);
        memcpy (&debug_dr16, &remoter, sizeof(msg_remoter_t));
        memcpy (&debug_ins, &ins, sizeof(msg_ins_t));
        tx_thread_sleep(1);
    }
}