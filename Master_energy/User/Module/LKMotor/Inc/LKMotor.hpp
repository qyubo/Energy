#ifndef LKMOTOR_HPP
#define LKMOTOR_HPP

#include "pid.hpp"
#include "main.h"

/**
 * @class LKMotor
 * @brief 电机控制类，提供电机的基本控制功能。
 *
 * 该类实现了电机的各种控制模式，包括速度、位置和基于IMU的控制。
 * 它还负责处理电机反馈数据和执行PID控制。
 */
#ifdef __cplusplus

	
class LKMotor
{
public:

    static constexpr float RawRpm2Radps = 0.1047197551196f; // 0.1047197551f;  // RPM 到弧度每秒的转换因子, 2 * PI / 60 (s)
    static constexpr float RawPos2Rad = 0.00009587379924285f;//0.0003834951969604492f; //TODO 2Pi/65536 位置值转换为弧度的转换因子，LK9025v2电机编码器为十六位，2^16 = 65535, 2 * PI / 65535 (rad)
    static constexpr float RawDps2Rpsps = 0.0174532925199433f; // 0.0174532925199433f; // DPS 到弧度每秒的转换因子, 2PI / 360 (rad)
    /**
     * @brief 定义电机测量数据的结构体。
     * @param last_ecd 上次电机编码器的读数。
     * @param ecd 当前电机编码器的读数。
     * @param speed_dps 电机的转速，单位为rpm。
     * @param given_current 给定的电机电流，单位为毫安。
     * @param temperate 电机的温度，单位为摄氏度。
     */
    typedef struct
    {
        uint16_t last_ecd;      ///< 上次电机编码器的读数
        uint16_t ecd;          ///< 当前电机编码器的读数
        int16_t speed_dps;     ///< 电机的转速，单位rpm
        int16_t given_current; ///< 给定的电机电流
        uint8_t temperate;     ///< 电机的温度
        
        uint8_t torqueFdb;     ///< 电机的转矩反馈

        uint8_t encoderOffset; ///< 电机零点处的编码器偏移
    } motor_measure_t;

    /**
     * @enum MotorControlModeType
     * @brief 描述电机的不同控制模式。
     */
    enum MotorControlModeType
    {
        RELAX_MODE = 0,          ///< 电机松开模式，所有输出均为0。
        SPD_MODE = 1,            ///< 速度模式，控制电机速度。
        POS_MODE = 2,            ///< 位置模式，控制电机到特定位置。
        POS_FOR_NO_SPD_MODE = 3, ///< 无速度反馈下的位置模式。
        IMU_MODE = 4,            ///< IMU模式，使用IMU反馈进行控制。
        TOR_MODE = 5             ///< 转矩模式，控制电机的转矩。
    };

    /**
     * @struct MotorFeedBack
     * @brief 电机反馈数据的结构体，包括电机的各种物理量反馈。
     * 结构体中包含了电机的电流、速度、位置等信息，以及电机的温度等状态反馈。
     */
    struct MotorFeedBack
    {
        int16_t last_ecd;      ///< 上次电机编码器的读数
        uint16_t ecd;          ///< 当前电机编码器的读数
        int16_t speed_dps;     ///< 电机的转速，单位rpm
        int16_t currentFdb;    ///< 电机电流反馈
        float speedFdb;        ///< 电机当前速度反馈, 单位rad/s
        float lastSpeedFdb;    ///< 上次记录的电机速度
        float positionFdb;     ///< 电机当前位置反馈
        float lastPositionFdb; ///< 上次记录的电机位置
        float temperatureFdb;  ///< 电机温度反馈
        float torqueFdb;       ///< 电机扭矩反馈
    };

    enum MotorType
    {
        LK9025_TYPE = 0,
        LK8016_TYPE
    };

    MotorType motorType; ///< 电机类型

    MotorControlModeType controlMode; ///< 当前电机控制模式
    MotorFeedBack motorFeedback;      ///< 电机的反馈数据
    uint16_t canId;                   ///< 电机的CAN通信ID
    CAN_HandleTypeDef *hcan;          ///< 指向电机使用的CAN接口的指针

    PID speedPid = PID(0.1f, 0.0f, 0.0f, 25000.0f, 3.0f, PID_POSITION);    ///< 速度环PID控制器
    PID positionPid = PID(0.1f, 0.0f, 0.0f, 25000.0f, 3.0f, PID_POSITION); ///< 位置环PID控制器


    float speedSet;    ///< 设定的目标速度
    float positionSet; ///< 设定的目标位置，范围[-Pi, Pi]
    float torqueSet; ///< 设定的目标转矩

    float offset;     ///< 电机的初始位置偏移
    
    int16_t currentSet;  ///< 设定的电流输出
    uint16_t maxCurrent; ///< 最大电流限制


    /**
     * @brief 纯虚函数，用于设置电机输出。
     * 必须在派生类中实现此函数。
     */
    virtual void setOutput() = 0;

    virtual void UpdateSensorData(uint8_t *buffer_ptr) = 0;

    /**
     * @brief 构造函数
     */
    LKMotor(MotorType type) : motorType(type)
    {
        controlMode = RELAX_MODE;

        speedSet = 0;
        positionSet = 0;
        currentSet = 0;
        torqueSet = 0;

        maxCurrent = 0;

        motorFeedback.speedFdb = 0;
        motorFeedback.lastSpeedFdb = 0;
        motorFeedback.positionFdb = 0;
        motorFeedback.lastPositionFdb = 0;
        motorFeedback.temperatureFdb = 0;

    }
};


#endif
#endif // LKMOTOR_HPP
