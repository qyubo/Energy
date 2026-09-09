#ifndef LK9025_HPP
#define LK9025_HPP

#include "main.h"
#include "LKMotor.hpp"


class LK9025 : public LKMotor
{
public:
    /**
     * @brief 构造函数，初始化GM6020电机控制类。
     */
    LK9025();

    /**
     * @brief 构造函数，初始化GM6020电机控制类。
     */
    ~LK9025();
    
    /**
     * @brief 实现电机输出设置。
     * 根据当前的控制模式和PID反馈调整电机输出。
     * 必须在派生类中具体实现此方法以适应具体电机。
     */
    void setOutput() override;

    void UpdateSensorData(uint8_t *buffer_ptr) override;
};

#endif // LK9025_HPP
