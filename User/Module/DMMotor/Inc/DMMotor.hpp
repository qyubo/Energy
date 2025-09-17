//
// Created by cosmosmount on 2025/8/30.
//

#ifndef RM26_DMMOTOR_HPP
#define RM26_DMMOTOR_HPP

#include "pid.hpp"

#define KP_MIN 0.0
#define KP_MAX 500.0

#define KD_MIN 0.0
#define KD_MAX 500.0

class DMMotor
{
private:
    float P_MIN; ///< 位置最小值
    float P_MAX; ///< 位置最大值

    float V_MIN; ///< 速度最小值
    float V_MAX; ///< 速度最大值

    float T_MIN; ///< 扭矩最小值
    float T_MAX; ///< 扭矩最大值

public:
    virtual ~DMMotor() = default;

    static constexpr uint8_t Enable_Frame[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC};           // 使能帧，DM电机需要初始化时发送该帧才能控制
    static constexpr uint8_t Disable_Frame[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD};          // 失能帧
    static constexpr uint8_t SaveZeroPosition_Frame[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE}; // 保存零点帧
    static constexpr uint8_t ClearError_Frame[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB};       // 清除错误帧

    enum MotorStateTypeDef
    {
        MOTOR_OFFLINE = 0, ///< 电机离线
        MOTOR_ONLINE = 1,  ///< 电机在线
    };

    /**
     * @enum MotorControlModeTypeDef
     * @brief 描述电机的不同控制模式。
     */
    enum MotorControlModeTypeDef
    {
        RELAX_MODE = 0,   ///< 电机松开模式
        MIT_MODE = 1,     ///< 达秒电机MIT模式
        POS_SPD_MODE = 2, ///< 位置速度模式，速度给定是梯形加速度运行下最高速度的，即为匀速段的速度值。
        SPD_MODE = 3,     ///< 速度模式
    };

    enum MotorErrorTypeDef
    {
        ERR_DISABLE = 0,         ///< 电机失能
        ERR_ENABLE = 1,          ///< 电机使能
        ERR_OVERVOLTAGE = 8,     ///< 电机过压
        ERR_UNDERVOLTAGE = 9,    ///< 电机欠压
        ERR_OVERCURRENT = 0xA,   ///< 电机过电流
        ERR_MOS_OVERTEMP = 0xB,  ///< M驱动上 MOS 过温
        ERR_COIL_OVERTEMP = 0xC, ///< 电机线圈过温
        ERR_COMM_LOST = 0xD,     ///< 通讯丢失
        ERR_OVERLOAD = 0xE,      ///< 过载
    };

    /**
     * @struct MotorFeedBack
     * @brief 电机反馈数据的结构体，包括电机的各种物理量反馈。
     * 结构体中包含了电机的电流、速度、位置等信息，以及电机的温度等状态反馈。
     */
    struct MotorFeedBackTypeDef
    {
        uint8_t ID;            ///< 电机反馈ID
        MotorErrorTypeDef ERR; ///< 电机状态码, 0：失能，1：使能，8：超压，9欠压，A：过电流，B：MOS过温，C：电机线圈过温，D：通讯丢失，E：过载
        float SpeedFdb;        ///< 电机当前速度反馈
        float PositionFdb;     ///< 电机当前位置反馈
        float TorqueFdb;       ///< 电机当前扭矩反馈
        float TemMOS;          ///< M驱动上 MOS 的平均温度，单位℃
        float TemRotor;        ///< 表示电机内部线圈的平均温度，单位℃
    };

    MotorControlModeTypeDef ControlMode; ///< 当前电机控制模式
    MotorFeedBackTypeDef MotorFeedback;  ///< 电机的反馈数据
    MotorStateTypeDef MotorState;        ///< 电机的状态
    uint32_t CAN_ID;                     ///< 电机的ID

    // 用于检测电机是否在线，需要在DMMotorHandler中和aliveCheck函数中处理
    uint32_t AliveFlag;
    uint32_t Pre_Flag;

    float SpeedSet;    ///< 设定的目标速度
    float PositionSet; ///< 设定的目标位置，范围[-Π, Π]
    float TorqueSet;   ///< 设定的目标扭矩

    DMMotor()
    {
        ControlMode = RELAX_MODE;
        MotorState = MOTOR_OFFLINE;

        SpeedSet = 0.0f;
        PositionSet = 0.0f;
        TorqueSet = 0.0f;
    };

    virtual float Get_P_MAX() const = 0;
    virtual float Get_P_MIN() const = 0;
    virtual float Get_V_MAX() const = 0;
    virtual float Get_V_MIN() const = 0;
    virtual float Get_T_MAX() const = 0;
    virtual float Get_T_MIN() const = 0;

    virtual MotorStateTypeDef AliveCheck() = 0;    // 检测电机是否在线，需要在主循环中调用
    virtual void SetOutput() = 0;                  // 设置电机输出
    virtual void ReceiveData(uint8_t *buffer) = 0; // 接收电机数据
};


#endif //RM26_DMMOTOR_HPP