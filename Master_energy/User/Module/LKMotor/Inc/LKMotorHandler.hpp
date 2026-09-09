#ifndef LKMOTORHANDLER_HPP
#define LKMOTORHANDLER_HPP

#include "main.h"

#include "bsp_can.hpp"
#include "LKMotor.hpp"
#include "pid.hpp"
#include "math.hpp"

#define LK_CAN_ID 0x280 ///< 瓴控电机的多电机控制指令，最多控制4个电机，回复标识符0x140 + 1~4

/**
 * @brief 电机控制类
 */
#ifdef __cplusplus

class LKMotorHandler
{
public:
    /**
     *@brief 使用指针数组, 2个CAN口，每个CAN口最多4个电机
     */
    LKMotor *LKMotorList[3][4]; ///<3路can

    uint32_t motor_loss_count_1[4];
    uint32_t motor_loss_count_2[4];

    LKMotorHandler();
    ~LKMotorHandler();

    // 电机数据转换因子
    const float RawRpm2Radps = 0.1047197551196f; // 0.1047197551f;  // RPM 到弧度每秒的转换因子, 2 * PI / 60 (s)
    const float RawPos2Rad = 0.00009587379924285f;//0.0003834951969604492f; //TODO 2Pi/65536 位置值转换为弧度的转换因子，LK9025v2电机编码器为十六位，2^16 = 65535, 2 * PI / 65535 (rad)
    const float RawDps2Rpsps = 0.0174532925199433f; // 0.0174532925199433f; // DPS 到弧度每秒的转换因子, 2PI / 360 (rad)

    /**
     * @brief CAN1接收数据，经过了初步的处理，索引对应电机ID
     */
    LKMotor::motor_measure_t can1_receive_data[4];
    /**
     * @brief CAN2接收数据，经过了初步的处理，索引对应电机ID
     */
    LKMotor::motor_measure_t can2_receive_data[4];
    /**
     * @brief CAN3接收数据，经过了初步的处理，索引对应电机ID
     */
    LKMotor::motor_measure_t can3_receive_data[4];

    uint8_t can1_send_data_0[8]; // CAN1电机控制数据，用于控制0x141-0x144

    uint8_t can2_send_data_0[8]; // CAN2电机控制数据，用于控制0x141-0x144

    uint8_t can3_send_data_0[8]; // CAN2电机控制数据，用于控制0x141-0x144

    /**
     * @brief 注册电机，将电机指针存入MotorList中
     * @param motor 电机指针
     * @param hcan CAN句柄
     * @param canId 电机ID
     */
    void registerMotor(LKMotor *LKmotor, CAN_HandleTypeDef *hcan, uint16_t canId); // 使用指针作为参数

    /**
     * @brief 处理来自can回调中原始数据
     * @param rx_data 接收到的原始数据
     * @param hcan CAN句柄
     * @param index 电机索引
     * @note 该函数用于处理CAN接收到的原始数据，将其转换为电机测量数据，储存在can_receive_data中。在最后调用updateFeedback()函数更新电机数据。
     */
    void processRawData(CAN_HandleTypeDef *hcan, uint8_t *rx_data, int index);

    void processZeroPointData(CAN_HandleTypeDef *hcan, uint8_t *rx_data, int index);

    /**
     * @brief 发送控制数据
     * @param hcan1 CAN1句柄
     * @param hcan2 CAN2句柄
     */
    void sendControlData();

    /**
     * @brief 处理并更新电机反馈数据
     * @param hcan1 CAN1句柄
     * @param hcan2 CAN2句柄
     */
    void updateFeedback(CAN_HandleTypeDef *hcan, uint8_t *rx_data, int index);

    void Receive(LKMotor *motor, uint8_t *can_receive_data);

    static LKMotorHandler *instance()
    {
        static LKMotorHandler instance;
        return &instance;
    }
};


#endif
#endif // LKMOTORHANDLER_HPP
