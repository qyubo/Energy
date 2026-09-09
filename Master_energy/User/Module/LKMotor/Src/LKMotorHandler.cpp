#include "LKMotorHandler.hpp"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern CAN_HandleTypeDef hcan3;


/**
 * @brief 构造函数，将所有值初始化
 */
LKMotorHandler::LKMotorHandler()
{
    for (int i = 0; i < 4; i++)
        LKMotorList[0][i] = nullptr;
    for (int i = 0; i < 4; i++)
        LKMotorList[1][i] = nullptr;
    for (int i = 0; i < 4; i++)
        LKMotorList[2][i] = nullptr;

    for (int i = 0; i < 4; i++)
        can1_send_data_0[i] = 0;
    for (int i = 0; i < 4; i++)
        can2_send_data_0[i] = 0;
    for (int i = 0; i < 4; i++)
        can3_send_data_0[i] = 0;
}

LKMotorHandler::~LKMotorHandler()
{
}

/**
 * @brief 注册电机，将电机指针存入MotorList中
 * @param motor 电机指针
 * @param hcan CAN句柄
 * @param canId 电机ID
 */
void LKMotorHandler::registerMotor(LKMotor *LKmotor, CAN_HandleTypeDef *hcan, uint16_t canId)
{
    LKmotor->canId = canId;
    LKmotor->hcan = hcan;

    if (canId >= 0x141 && canId <= 0x144)
    {
        if (LKmotor->hcan == &hcan1)
            LKMotorList[0][canId - 0x141] = LKmotor;
        else if (LKmotor->hcan == &hcan2)
            LKMotorList[1][canId - 0x141] = LKmotor;
        else if (LKmotor->hcan == &hcan3)
            LKMotorList[2][canId - 0x141] = LKmotor;
    }
}

// void LKMotorHandler::processRawData(CAN_HandleTypeDef *hcan, uint8_t *rx_data, int index)
// {
//     if (hcan == &hcan1)
//     {
//         for (int i = 0; i < 4; i++)
//         {
//             can1_receive_data[index].last_ecd = can1_receive_data[index].ecd;                 ///< 上一次的编码器值
//             can1_receive_data[index].ecd = (uint16_t)(rx_data[7] << 8 | rx_data[6]);          ///< 编码器值
//             can1_receive_data[index].speed_dps = (int16_t)(rx_data[5] << 8 | rx_data[4]);     ///< 速度值，单位dps, degree per second
//             can1_receive_data[index].given_current = (int16_t)(rx_data[3] << 8 | rx_data[2]); ///< 电流值，或者说是转矩值
//             can1_receive_data[index].temperate = rx_data[1];                                  ///< 温度值
//             // motor_loss_count_1[index]++;
//         }
//     }
//     else if (hcan == &hcan2)
//     {
//         for (int i = 0; i < 4; i++)
//         {
//             can2_receive_data[index].last_ecd = can2_receive_data[index].ecd;                 ///< 上一次的编码器值
//             can2_receive_data[index].ecd = (uint16_t)(rx_data[7] << 8 | rx_data[6]);          ///< 编码器值
//             can2_receive_data[index].speed_dps = (int16_t)(rx_data[5] << 8 | rx_data[4]);     ///< 速度值，单位dps, degree per second
//             can2_receive_data[index].given_current = (int16_t)(rx_data[3] << 8 | rx_data[2]); ///< 电流值，或者说是转矩值
//             can2_receive_data[index].temperate = rx_data[1];                                  ///< 温度值
//             // motor_loss_count_2[index]++;
//         }
//     }
//     else if (hcan == &hcan3)
//     {
//         for (int i = 0; i < 4; i++)
//         {
//             can3_receive_data[index].last_ecd = can3_receive_data[index].ecd;                 ///< 上一次的编码器值
//             can3_receive_data[index].ecd = (uint16_t)(rx_data[7] << 8 | rx_data[6]);          ///< 编码器值
//             can3_receive_data[index].speed_dps = (int16_t)(rx_data[5] << 8 | rx_data[4]);     ///< 速度值，单位dps, degree per second
//             can3_receive_data[index].given_current = (int16_t)(rx_data[3] << 8 | rx_data[2]); ///< 电流值，或者说是转矩值
//             can3_receive_data[index].temperate = rx_data[1];                                  ///< 温度值
//             // motor_loss_count_2[index]++;
//         }
//     }

//     updateFeedback();
// }





/**
 * @brief 发送控制数据
 * @param hcan1 CAN1句柄
 * @param hcan2 CAN2句柄
@attention 瓴控电机电流高低位存储位置和大疆电机不同
 */
void LKMotorHandler::sendControlData()
{
    // 循环遍历所有的电机，将电机的控制数据存入can_send_data中
    for (int i = 0; i < 4; i++)
    {
        // 处理挂载在CAN1的电机
        if (LKMotorList[0][i] != nullptr)
        {
            // 0x141-144，控制标识符为0x280
            if (LKMotorList[0][i]->canId >= 0x141 && LKMotorList[0][i]->canId <= 0x144)
            {
                int index = (LKMotorList[0][i]->canId - 0x140) * 2;
                can1_send_data_0[index - 2] = LKMotorList[0][i]->currentSet;
                can1_send_data_0[index - 1] = LKMotorList[0][i]->currentSet >> 8;
            }

        }
        // 处理挂载在CAN2的电机
        if (LKMotorList[1][i] != nullptr)
        {
            // 0x141-144，控制标识符为0x280
            if (LKMotorList[1][i]->canId >= 0x141 && LKMotorList[1][i]->canId <= 0x144)
            {
                int index = (LKMotorList[1][i]->canId - 0x140) * 2;
                can2_send_data_0[index - 2] = LKMotorList[1][i]->currentSet;
                can2_send_data_0[index - 1] = LKMotorList[1][i]->currentSet >> 8;
            }

        }
        // 处理挂载在CAN2的电机
        if (LKMotorList[2][i] != nullptr)
        {
            // 0x141-144，控制标识符为0x280
            if (LKMotorList[2][i]->canId >= 0x141 && LKMotorList[2][i]->canId <= 0x144)
            {
                int index = (LKMotorList[2][i]->canId - 0x140) * 2;
                can3_send_data_0[index - 2] = LKMotorList[2][i]->currentSet;
                can3_send_data_0[index - 1] = LKMotorList[2][i]->currentSet >> 8;
            }

        }
    }
    // 使用bsp_can中的函数发送数据
    CAN_Transmit(&hcan1, 0x280, can1_send_data_0, 8);

    CAN_Transmit(&hcan2, 0x280, can2_send_data_0, 8);

    CAN_Transmit(&hcan3, 0x280, can3_send_data_0, 8);
}

/**
 * @brief 处理并更新电机反馈数据
 * 将电机的反馈数据存入Motorlist中存在的电机的MotorFeedback中
 * @param hcan1 CAN1句柄
 * @param hcan2 CAN2句柄
 */


void LKMotorHandler::updateFeedback(CAN_HandleTypeDef *hcan, uint8_t *rx_data, int index)
{
    if (hcan == &hcan1)
    {
        //< 如果电机列表的最大数量发生变化，这里的循环次数需要修改！！！
        for (int i = 0; i < 4; i++)
        {
            // 处理CAN1的电机
            if (LKMotorList[0][index] != nullptr)
            {
                Receive(LKMotorList[0][index], rx_data);
            }
        }
    }

    else if (hcan == &hcan2)
    {
        for (int i = 0; i < 4; i++)
        {
            // 处理CAN2的电机
            if (LKMotorList[1][index] != nullptr)
            {
                Receive(LKMotorList[1][index], rx_data);
            }
        }
    }

    else if (hcan == &hcan3)
    {
        for (int i = 0; i < 4; i++)
        {
            // 处理CAN3的电机
            if (LKMotorList[2][index] != nullptr)
            {
                Receive(LKMotorList[2][index], rx_data);
            }
        }
    }
}

/**
 * @brief 接收电机数据，将数据存入电机的反馈数据中
 * @param motor 电机指针
 * @param can_rx_buff CAN接收数据
 */
void LKMotorHandler::Receive(LKMotor *motor, uint8_t *can_receive_data)
{
    if (motor != nullptr)
    {
        motor->UpdateSensorData(can_receive_data);
    }
}