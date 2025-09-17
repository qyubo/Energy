#include "ServiceMotor.hpp"
#include "bsp_can.hpp"
#include "main.h"


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
    LWheel.controlMode = GM6020::RELAX_MODE;
    LWheel.setOutput();
    DJIMotorhandler->registerMotor(&LWheel, &hcan1, 0x205);

    RWheel.controlMode = M2006::RELAX_MODE;
    RWheel.setOutput();
    DJIMotorhandler->registerMotor(&RWheel, &hcan1, 0x202);


}

void ServiceMotors::AllMotorSetOutput()
{
    LWheel.setOutput();
    RWheel.setOutput();
}

void ServiceMotors::SetModeAndPidParam()
{
    LWheel.controlMode = GM6020::SPD_MODE;
    RWheel.controlMode = M2006::SPD_MODE;

    // LWheel.speedPid.mode = PID_POSITION | PID_Integral_Limit | PID_Trapezoid_Intergral;
    // LWheel.speedPid.kp = 200.0f;
    // LWheel.speedPid.ki = 0.0f;
    // LWheel.speedPid.kd = 4000.0f;
    // LWheel.speedPid.ScalarA = 6.0f;
    // LWheel.speedPid.ScalarB = 0.01f;
    // LWheel.speedPid.maxIOut = 10.0f;
    // LWheel.speedPid.maxOut = 10000.0f;

    RWheel.speedPid.mode = PID_POSITION | PID_Integral_Limit | PID_Trapezoid_Intergral;
    RWheel.speedPid.kp = 200.0f;
    RWheel.speedPid.ki = 0.0f;
    RWheel.speedPid.kd = 4000.0f;
    RWheel.speedPid.ScalarA = 6.0f;
    RWheel.speedPid.ScalarB = 0.01f;
    RWheel.speedPid.maxIOut = 10.0f;
    RWheel.speedPid.maxOut = 10000.0f;
}



[[noreturn]] void MotorThreadFun(ULONG initial_input) {
    //TODO:接收onemessage控制信息

    UNUSED(initial_input);
    ULONG time;

    //注册电机
    ServiceMotors serviceMotors;
    serviceMotors.MotorRegister();

    //TODO： 从control_task来的om topic发送控制信息
    //TODO: param may be om_published outcome
    serviceMotors.SetModeAndPidParam();


    for (;;) {
        time = tx_time_get();

        //TODO:电机接收控制信号、保护与使能失能
        // bool update_flag[6] = {0, 0, 0, 0, 0,0};
        // if (om_suber_export(motor_recv_suber, &motor_ctr, false) == OM_OK) {
        //     last_topic_time = tx_time_get();
        //     if (motor_ctr.enable) {
        //         for (uint8_t i = 0; i < 6; i++) {
        //             MotorUnit[i].SetTorque(motor_ctr.torque[i]);
        //         }
        //     } else {
        //         for (auto &motor: MotorUnit) {
        //             motor.SetTorque(0);
        //         }
        //     }
        // } else if (tx_time_get() - last_topic_time > 500) {
        //     for (auto &motor: MotorUnit) {
        //         motor.SetTorque(0);
        //     }
        // }

        // /*Motor disable or enable*/
        // if (last_enable != motor_ctr.enable) {
        //     if (motor_ctr.enable) {
        //         for (auto &motor: MotorUnit) {
        //             motor.EnableMotor();
        //             tx_thread_sleep(1);
        //         }
        //     } else {
        //         for (auto &motor: MotorUnit) {
        //             motor.DisableMotor();
        //             tx_thread_sleep(1);
        //         }
        //     }
        // }
        // last_enable = motor_ctr.enable;

        //for test
        // serviceMotors.LWheel.speedSet = 0.5;
        serviceMotors.RWheel.speedSet = 1.0;

        // RxData1.cnt = 0;
        // RxData2.cnt = 0;
        // CAN_cnt = 0;

        //计算电流值
        serviceMotors.AllMotorSetOutput();
        serviceMotors.LWheel.currentSet = test_cur;
        // serviceMotors.RWheel.currentSet = 10;

        //TODO: 补全电机掉线处理或输出
        // if (tx_semaphore_get(&MotorCANRecvSem, 1)) {
        //
        // }

        //TODO: 解析电机数据
            //在can回调中断中执行
//         uint8_t id;
//         for (int i = 0; i < 3; ++i) {
//             id = static_cast<uint8_t>(RxData1.data[i][0] & 0x0F) - 1;
//             MotorUnit[id].MessageDecode(RxData1.data[i]);
// //            update_flag[id] = true;
//         }
//         for (int i = 0; i < 3; ++i) {
//             id = static_cast<uint8_t>(RxData2.data[i][0] & 0x0F) - 1;
//             MotorUnit[id].MessageDecode(RxData2.data[i]);
// //            update_flag[id] = true;
//         }

        //TODO： 获得电机回调数据并publish出去



        // om_publish(link_topic, &link_msg, sizeof(link_msg), true, false);

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
