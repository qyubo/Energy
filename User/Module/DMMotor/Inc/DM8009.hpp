//
// Created by cosmosmount on 2025/8/30.
//

#ifndef RM26_DM8009_HPP
#define RM26_DM8009_HPP

#include "DMMotor.hpp"

class DM8009 : public DMMotor
{
private:
    float P_MIN; ///< 位置最小值
    float P_MAX; ///< 位置最大值

    float V_MIN; ///< 速度最小值
    float V_MAX; ///< 速度最大值

    float T_MIN; ///< 扭矩最小值
    float T_MAX; ///< 扭矩最大值

public:
    float LowerPosLimit; // 电机位置下限
    float UpperPosLimit; // 电机位置上限
    float Offset;        // 电机位置偏移

    float KP;
    float KD;
    DM8009();
    virtual ~DM8009() = default;
    [[nodiscard]] inline float Get_P_MAX() const override { return P_MAX; };
    [[nodiscard]] inline float Get_P_MIN() const override { return P_MIN; };
    [[nodiscard]] inline float Get_V_MAX() const override { return V_MAX; };
    [[nodiscard]] inline float Get_V_MIN() const override { return V_MIN; };
    [[nodiscard]] inline float Get_T_MAX() const override { return T_MAX; };
    [[nodiscard]] inline float Get_T_MIN() const override { return T_MIN; };

    MotorStateTypeDef AliveCheck() override;

    void SetOutput() override;                  // 设置电机输出
    void ReceiveData(uint8_t *buffer) override; // 接收电机数据
};

#endif //RM26_DM8009_HPP