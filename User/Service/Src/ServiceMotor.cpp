#include "main.h"

#include "ServiceMotor.hpp"
#include "om.h"
#include "magicmsgs.hpp"


extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern CAN_HandleTypeDef hcan3;

TX_SEMAPHORE MotorCANRecvSem;

TX_THREAD MotorThread;
uint8_t MotorThreadStack[4096] = {0};
DJIMotorHandler* DJIMotorhandler = DJIMotorHandler::Instance();

int16_t test_cur = 0;

void ServiceMotors::MotorRegister() {
    // //注册电机
    YawMotor.currentSet = 0;
    DJIMotorhandler->registerMotor(&YawMotor, &hcan1, 0x205);

    PitchMotor.currentSet = 0;
    DJIMotorhandler->registerMotor(&PitchMotor, &hcan1, 0x206);


}

void ServiceMotors::AllMotorSetOutput()
{
    // LWheel.setOutput();
    // RWheel.setOutput();
}

void ServiceMotors::SetModeAndPidParam()
{
    // LWheel.controlMode = GM6020::SPD_MODE;
    // RWheel.controlMode = M2006::SPD_MODE;
    //
    // // LWheel.speedPid.mode = PID_POSITION | PID_Integral_Limit | PID_Trapezoid_Intergral;
    // // LWheel.speedPid.kp = 200.0f;
    // // LWheel.speedPid.ki = 0.0f;
    // // LWheel.speedPid.kd = 4000.0f;
    // // LWheel.speedPid.ScalarA = 6.0f;
    // // LWheel.speedPid.ScalarB = 0.01f;
    // // LWheel.speedPid.maxIOut = 10.0f;
    // // LWheel.speedPid.maxOut = 10000.0f;
    //
    // RWheel.speedPid.mode = PID_POSITION | PID_Integral_Limit | PID_Trapezoid_Intergral;
    // RWheel.speedPid.kp = 200.0f;
    // RWheel.speedPid.ki = 0.0f;
    // RWheel.speedPid.kd = 4000.0f;
    // RWheel.speedPid.ScalarA = 6.0f;
    // RWheel.speedPid.ScalarB = 0.01f;
    // RWheel.speedPid.maxIOut = 10.0f;
    // RWheel.speedPid.maxOut = 10000.0f;
}



[[noreturn]] void MotorThreadFun(ULONG initial_input) {
    UNUSED(initial_input);
    ULONG time;

    //注册电机
    ServiceMotors serviceMotors;
    serviceMotors.MotorRegister();

    om_suber_t *gimbal_suber = om_subscribe(om_find_topic("gimbalctrl", UINT32_MAX));
    msg_gimbal_ctrl_t gimbal_ctrl{};
    serviceMotors.SetModeAndPidParam();

    for (;;) {
        time = tx_time_get();
        om_suber_export(gimbal_suber, &gimbal_ctrl, false);
        serviceMotors.YawMotor.currentSet = static_cast<int16_t>(gimbal_ctrl.yaw_torque*100);
        serviceMotors.PitchMotor.currentSet = static_cast<int16_t>(gimbal_ctrl.pitch_torque*100);

        //发送控制指令给电机
        DJIMotorhandler->sendControlData();

        //比较准确的执行时间为1ms，可能不需要
        uint8_t time_to_delay = tx_time_get() - time;
        if (time_to_delay < 1) {
            tx_thread_sleep(1 - time_to_delay);
        }
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
        // Monitor::Instance()->Log_Messages(Monitor::ERROR, (uint8_t *)"CAN Unknown ID\r\n", USART_MODE_DMA);
    }
}
