#include "main.h"

#include "ServiceMotor.hpp"
#include "om.h"
#include "magicmsgs.hpp"
#include "filter.hpp"
#include "math.hpp"
#include "bsp_dwt.hpp"

using namespace Filter;
using namespace Numeric;

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern CAN_HandleTypeDef hcan3;

TX_SEMAPHORE MotorCANRecvSem;

TX_THREAD MotorThread;
uint8_t MotorThreadStack[4096] = {0};
DJIMotorHandler* DJIMotorhandler = DJIMotorHandler::Instance();

motor_debug_t motor_debug;
pid_tuning_t motor_pos_pid;
pid_tuning_t motor_spd_pid;
float debug_cur = 0.0f;
float vel_ratio = 0;
float test_r = 0;
float debug_set = 0.0f;

void ServiceMotors::MotorRegister() {
    // //注册电机
    DJIMotorhandler->registerMotor(&YawMotor, &hcan1, 0x206);
    YawMotor.currentSet = 0;
    YawMotor.gearBox = GearBox_None;

    DJIMotorhandler->registerMotor(&PitchMotor, &hcan1, 0x205);
    PitchMotor.currentSet = 0;
    PitchMotor.gearBox = GearBox_None;
    PitchMotor.P_MIN = -1.66360223f;
    PitchMotor.P_MAX = -1.0262332f;
}

void ServiceMotors::SetModeAndPidParam()
{
    YawMotor.speedPid.kp = 300.0f;
    YawMotor.speedPid.ki = 0.01f;
    YawMotor.speedPid.kd = 1.0f;

    PitchMotor.speedPid.kp = 100.0f;
}

[[noreturn]] void MotorThreadFun(ULONG initial_input) {
    UNUSED(initial_input);
    ULONG time;

    om_suber_t *gimbal_suber = om_subscribe(om_find_topic("gimbalctrl", UINT32_MAX));
    msg_gimbal_ctrl_t gimbal_ctrl{};

    for (;;) {
        om_suber_export(gimbal_suber, &gimbal_ctrl, false);
        tx_thread_sleep(1);
    }
}

/**
 * @brief CAN接收中断回调函数，所有反馈在can上的数据会在这里根据ID进行分类并处理。
 * @param hcan CAN句柄
 * @note 该函数用于处理CAN接收中断，根据ID分类处理接收到的数据。但是这中方法可能会在回调里浪费时间，因为这里的处理是阻塞的。考虑是否需要将数据存储到一个缓冲区，然后在主循环中处理。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    // 接收数据
    uint8_t rx_data[8];
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);

    /*-------------------------------------------------大疆电机数据-------------------------------------------------*/
    if (rx_header.StdId >= 0x201 && rx_header.StdId <= 0x208)
    {
        if (hcan->Instance == CAN1)
        {
            DJIMotorHandler::Instance()->updateFeedback(hcan, rx_data, int(rx_header.StdId - 0x201));
        }
        else if (hcan->Instance == CAN2) // 处理CAN2的数据
        {
            DJIMotorHandler::Instance()->updateFeedback(hcan, rx_data, int(rx_header.StdId - 0x201));
        }
    }
    else // 未知的ID，需要进行错误处理
    {
    }
}
