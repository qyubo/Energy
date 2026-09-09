//
// Created by cosmosmount on 2025/9/8.
//

#include "main.h"
#include "tx_api.h"
#include "led.hpp"
#include "ServiceBooster.hpp"
#include "ServiceRemoter.hpp"

#include "bsp_can.hpp"
#include "can_portocal_first.hpp"
#include "bsp.hpp"//
#include <stdlib.h>




extern TX_THREAD RemoterThread;
extern TX_SEMAPHORE RemoterThreadSem;
extern uint8_t RemoterThreadStack[2048];
extern void RemoterThreadFun(ULONG initial_input);


// extern TX_THREAD EnergyThread;
// extern TX_SEMAPHORE EnergySem;
// extern uint8_t EnergyThreadStack[4096];
// extern void EnergyThreadFun(ULONG initial_input);
/*EKF pool*/
TX_BYTE_POOL MathPool;
UCHAR Math_PoolBuf[14336] = {0};

/*OneMessage pool*/
TX_BYTE_POOL MsgPool;
UCHAR Msg_PoolBuf[4096] = {0};






#define TX_NAME(s) const_cast<CHAR*>(s)

void ServiceBooster()
{
    /*Math pool in ccram*/
    tx_byte_pool_create(
            &MathPool,
            TX_NAME("Math_Pool"),
            Math_PoolBuf,
            sizeof(Math_PoolBuf));

    tx_byte_pool_create(
            &MsgPool,
            TX_NAME("Msg_Pool"),
            Msg_PoolBuf,
            sizeof(Msg_PoolBuf));

   
    tx_semaphore_create(&RemoterThreadSem, TX_NAME("RemoterThreadSem"), 0);

    /* Create energy_thread! */

    
    tx_thread_create(&RemoterThread, TX_NAME("RemoterThread"),
        RemoterThreadFun, 0x1234, RemoterThreadStack, sizeof(RemoterThreadStack),
        4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

}