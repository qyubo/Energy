#include "TaskBooster.hpp"

#include "main.h"
#include "tx_api.h"

extern TX_THREAD GimbalThread;
extern uint8_t GimbalThreadStack[4096];
extern void GimbalThreadFun(ULONG initial_input);

extern TX_THREAD ChassisThread;
extern uint8_t ChassisThreadStack[4096];
extern void ChassisThreadFun(ULONG initial_input);

#define TX_NAME(s) const_cast<CHAR*>(s)
void TaskBooster(void)
{
    /* Create the main thread.  */
    tx_thread_create(&GimbalThread, TX_NAME("GimbalThread"), GimbalThreadFun, 0x1234,
                     GimbalThreadStack, sizeof(GimbalThreadStack),
                     6, 6, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&ChassisThread, TX_NAME("ChassisThread"), ChassisThreadFun, 0x1234,
                     ChassisThreadStack, sizeof(ChassisThreadStack),
                     7, 7, TX_NO_TIME_SLICE, TX_AUTO_START);
}