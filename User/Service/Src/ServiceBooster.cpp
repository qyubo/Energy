//
// Created by cosmosmount on 2025/9/8.
//

#include "main.h"
#include "tx_api.h"
#include "led.hpp"
#include "ServiceBooster.hpp"

TX_THREAD my_thread1;
uint8_t my_thread_stack1[1024];
TX_SEMAPHORE my_semaphore1;

TX_THREAD my_thread2;
uint8_t my_thread_stack2[1024];

extern TX_THREAD RemoterThread;
extern TX_SEMAPHORE RemoterThreadSem;
extern uint8_t RemoterThreadStack[2048];
extern void RemoterThreadFun(ULONG initial_input);

extern TX_THREAD IMUThread;
extern TX_SEMAPHORE IMUThreadSem;
extern uint8_t IMUThreadStack[4096];
extern void IMUThreadFun(ULONG initial_input);

extern TX_THREAD IMUTempThread;
extern uint8_t IMUTempThreadStack[2048];
extern void IMUTempThreadFun(ULONG initial_input);

extern TX_THREAD MotorThread;
extern uint8_t MotorThreadStack[4096];
extern void MotorThreadFun(ULONG initial_input);

/*EKF pool*/
TX_BYTE_POOL MathPool;
UCHAR Math_PoolBuf[14336] = {0};

/*OneMessage pool*/
TX_BYTE_POOL MsgPool;
UCHAR Msg_PoolBuf[4096] = {0};

[[noreturn]] void my_thread_entry(ULONG thread_input)
{
    LED_ALL_ON();
    /* Enter into a forever loop. */
    while(1)
    {

        /* Increment thread counter. */
        tx_semaphore_put(&my_semaphore1);
        /* Sleep for 1 tick. */
        tx_thread_sleep(1000);
    }
}

[[noreturn]] void my_thread_entry2(ULONG thread_input)
{
    /* Enter into a forever loop. */
    while(1)
    {
        /* Increment thread counter. */
        if (tx_semaphore_get(&my_semaphore1, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            LED_blink();
            /* Sleep for 1 tick. */
        }
    }
}

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

    tx_semaphore_create(&my_semaphore1, TX_NAME("my_semaphore1"), 0);
    tx_semaphore_create(&RemoterThreadSem, TX_NAME("RemoterThreadSem"), 0);
    tx_semaphore_create(&IMUThreadSem, TX_NAME("IMUThreadSem"), 0);

    /* Create my_thread! */
    tx_thread_create(&my_thread1, TX_NAME("my_thread1"),
        my_thread_entry, 0x1234, my_thread_stack1, sizeof(my_thread_stack1),
        10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&my_thread2, TX_NAME("my_thread2"),
        my_thread_entry2, 0x1234, my_thread_stack2, sizeof(my_thread_stack2),
        10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&RemoterThread, TX_NAME("RemoterThread"),
        RemoterThreadFun, 0x1234, RemoterThreadStack, sizeof(RemoterThreadStack),
        4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&IMUThread, TX_NAME("IMUThread"),
        IMUThreadFun, 0x1234, IMUThreadStack, sizeof(IMUThreadStack),
        2, 2, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&IMUTempThread, TX_NAME("IMUTempThread"),
        IMUTempThreadFun, 0x1234, IMUTempThreadStack, sizeof(IMUTempThreadStack),
        5, 5, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_thread_create(&MotorThread, TX_NAME("MotorThread"),
        MotorThreadFun, 0x1234, MotorThreadStack, sizeof(MotorThreadStack),
        2, 2, TX_NO_TIME_SLICE, TX_AUTO_START);
}