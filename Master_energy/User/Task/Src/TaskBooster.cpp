#include "TaskBooster.hpp"

#include "main.h"
#include "tx_api.h"

extern TX_THREAD EnergyThread;
extern TX_SEMAPHORE EnergySem;
extern uint8_t EnergyThreadStack[4096];
extern void EnergyThreadFun(ULONG initial_input);

#define TX_NAME(s) const_cast<CHAR*>(s)
void TaskBooster(void)
{
     tx_semaphore_create(&EnergySem, TX_NAME("EnergySem"), 0);
    /* Create the main thread.  */
    tx_thread_create(&EnergyThread, TX_NAME("EnergyThread"), EnergyThreadFun, 0x1234,
                     EnergyThreadStack, sizeof(EnergyThreadStack),
                     6, 6, TX_NO_TIME_SLICE, TX_AUTO_START);

}