//
// Created by cosmosmount on 2025/8/30.
//

#include "DM4310.hpp"
#include "math.hpp"
using namespace Numeric;

DM4310::DM4310()
{
    ControlMode = RELAX_MODE;
    SpeedSet = 0.0f;
    PositionSet = 0.0f;
    MotorFeedback.ID = 0;
    MotorFeedback.ERR = ERR_DISABLE;
    MotorFeedback.SpeedFdb = 0.0f;
    MotorFeedback.PositionFdb = 0.0f;
    MotorFeedback.TorqueFdb = 0.0f;
    MotorFeedback.TemMOS = 0.0f;
    MotorFeedback.TemRotor = 0.0f;
    P_MAX = 12.56637061f;
    P_MIN = -12.56637061f;

    V_MAX = 15.0f;
    V_MIN = -15.0f;

    T_MAX = 10.0f;
    T_MIN = -10.0f;
    AliveFlag = 0;
    Pre_Flag = 0;

    MotorState = MOTOR_OFFLINE;

    LowerPosLimit = P_MIN;
    UpperPosLimit = P_MAX;
}

DM4310::MotorStateTypeDef DM4310::AliveCheck()
{
    if (AliveFlag == Pre_Flag)
    {
        MotorState = MOTOR_OFFLINE;
    }
    else
    {
        Pre_Flag = AliveFlag;
        MotorState = MOTOR_ONLINE;
    }
    return MotorState;
}

void DM4310::SetOutput()
{
    uint8_t OutputData[8] = {0};
    uint32_t CAN_ID = this->CAN_ID;

    this->PositionSet = FloatConstrain(this->PositionSet, this->LowerPosLimit, this->UpperPosLimit);
    switch (this->ControlMode)
    {
    case DMMotor::RELAX_MODE:
    {
        memcpy(OutputData, Disable_Frame, 8);
        break;
    }
    case DMMotor::MIT_MODE:
    {
        float postion_set = LoopFloatConstrain(this->PositionSet + this->Offset, -Pi, Pi);
        uint16_t pos_tmp = float_to_uint(postion_set, this->Get_P_MIN(), this->Get_P_MAX(), 16);
        uint16_t vel_tmp = float_to_uint(this->SpeedSet, this->Get_V_MIN(), this->Get_V_MAX(), 12);
        uint16_t kp_tmp = float_to_uint(this->KP, KP_MIN, KP_MAX, 12);
        uint16_t kd_tmp = float_to_uint(this->KD, KD_MIN, KD_MAX, 12);
        uint16_t tor_tmp = float_to_uint(this->TorqueSet, this->Get_T_MIN(), this->Get_T_MAX(), 12);
        OutputData[0] = (pos_tmp >> 8);
        OutputData[1] = pos_tmp;
        OutputData[2] = (vel_tmp >> 4);
        OutputData[3] = ((vel_tmp & 0xF) << 4) | (kp_tmp >> 8);
        OutputData[4] = kp_tmp;
        OutputData[5] = (kd_tmp >> 4);
        OutputData[6] = ((kd_tmp & 0xF) << 4) | (tor_tmp >> 8);
        OutputData[7] = tor_tmp;
        break;
    }
    case DMMotor::POS_SPD_MODE:
    {
        CAN_ID += 0x100;
        float postion_set = LoopFloatConstrain(this->PositionSet + this->Offset, -Pi, Pi);
        auto const *pbuf = reinterpret_cast<uint8_t *>(&postion_set);
        auto const *vbuf = reinterpret_cast<uint8_t *>(&this->SpeedSet);

        OutputData[0] = *pbuf;
        OutputData[1] = *(pbuf + 1);
        OutputData[2] = *(pbuf + 2);
        OutputData[3] = *(pbuf + 3);
        OutputData[4] = *vbuf;
        OutputData[5] = *(vbuf + 1);
        OutputData[6] = *(vbuf + 2);
        OutputData[7] = *(vbuf + 3);
        break;
    }
    case DMMotor::SPD_MODE:
    {
        CAN_ID += 0x200;
        auto const *vbuf = reinterpret_cast<uint8_t *>(&this->SpeedSet);
        OutputData[0] = *vbuf;
        OutputData[1] = *(vbuf + 1);
        OutputData[2] = *(vbuf + 2);
        OutputData[3] = *(vbuf + 3);
        break;
    }
    default:
    {
        memcpy(OutputData, Disable_Frame, 8);
        break;
    }
    }

}
void DM4310::ReceiveData(uint8_t *buffer)
{
    this->AliveFlag++; // 电机在线标志
    this->MotorFeedback.ID = buffer[0] & 0x0F;
    this->MotorFeedback.ERR = static_cast<DMMotor::MotorErrorTypeDef>(buffer[0] >> 4);

    // 提取位置、速度、扭矩的整型值
    uint16_t p_int = (buffer[1] << 8) | buffer[2];
    uint16_t v_int = (buffer[3] << 4) | (buffer[4] >> 4);
    uint16_t t_int = ((buffer[4] & 0x0F) << 8) | buffer[5];

    // 使用 uint_to_float 进行转换
    this->MotorFeedback.PositionFdb = LoopFloatConstrain(uint_to_float(p_int, this->Get_P_MIN(), this->Get_P_MAX(), 16), -Pi, Pi);
    this->MotorFeedback.PositionFdb = LoopFloatConstrain(this->MotorFeedback.PositionFdb - this->Offset, -Pi, Pi);
    this->MotorFeedback.SpeedFdb = uint_to_float(v_int, this->Get_V_MIN(), this->Get_V_MAX(), 12);
    this->MotorFeedback.TorqueFdb = uint_to_float(t_int, this->Get_T_MIN(), this->Get_T_MAX(), 12);
    // 温度信息
    this->MotorFeedback.TemMOS = buffer[6];
    this->MotorFeedback.TemRotor = buffer[7];
}