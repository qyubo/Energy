#pragma once
#ifndef TASKWHEEL_HPP
#define TASKWHEEL_HPP

#include "cstdint"
#include "tx_api.h"


#include "DJIMotorHandler.hpp"
#include "LKMotorHandler.hpp"
#include "LK9025.hpp"
#include "LK8016.hpp"
#include "GM6020.hpp"
#include "M3508.hpp"
#include "M2006.hpp"


class ServiceMotors
{
public:
    // LK9025 RMotor;
    // LK9025 LMotor;

    // LK8016 RD;
    // LK8016 RU;

    // LK8016 LD;
    // LK8016 LU;

    GM6020 YawMotor;
    GM6020 PitchMotor;

    void MotorRegister();
    void AllMotorSetOutput();
    void SetModeAndPidParam();

    static ServiceMotors *Instance()
    {
        static ServiceMotors instance;
        return &instance;
    }
};


#endif