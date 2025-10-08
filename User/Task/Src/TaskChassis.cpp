//
// Created by ASUS on 2025/10/7.
//

#include <iterator>

#include "main.h"
#include "tx_api.h"

#include "om.h"
#include "pid.hpp"
#include "math.hpp"
#include "slope.hpp"
#include "bsp_can.hpp"
#include "magicmsgs.hpp"
#include "ServiceMotor.hpp"

using namespace Math;

TX_THREAD ChassisThread;
uint8_t ChassisThreadStack[4096] = {0};
uint8_t debug_mode;

extern CAN_HandleTypeDef hcan2;

#pragma pack(push,1)

typedef enum : uint8_t
{
    NONE = 0,
    NORMAL_MOVING_MODE,
    ESCAPE_MODE,
    ABNORMAL_MOVING_MODE
} chassis_mode_e;

typedef enum : uint8_t
{
    NORMAL_FOLLOW_ROTATE = 0,
    SPIN_ROTATE
} rotate_type_e;

typedef enum
{
    DO_NOT_JUMP = 0,
    JUMP_READY,
    JUMP_START
} jump_ctrl_e;

typedef enum
{
    NOT_FLY_MODE = 0,
    FLY_MODE
} fly_ctrl_e;

__PACKED_STRUCT chassis_state_t
{
    chassis_mode_e NormalState : 2;
    rotate_type_e RotateState : 1;
    jump_ctrl_e jump_ctrl : 2;
    fly_ctrl_e fly_ctrl : 1;
};

#pragma pack(pop)

[[noreturn]] void ChassisThreadFun(ULONG initial_input)
{
    UNUSED(initial_input);

    om_topic_t* chassismsg_topic = om_config_topic(nullptr, "ca", "chassis", sizeof(msg_chassis_t));
    om_suber_t* remoter_suber = om_subscribe(om_find_topic("remoter", UINT32_MAX));
    msg_remoter_t remoter{};
    msg_chassis_t chassis_msg{};
    float Vlen_raw = 0.0f;
    float Vy_raw = 0.0f;

    SLOPE Vlen_slope;
    SLOPE Vy_slope;

    Vlen_slope.Init(0.005f, 0.005f);
    Vy_slope.Init(0.1f, 0.1f);

    chassis_state_t chassisState{};
    chassisState.NormalState = NONE;
    chassisState.RotateState = NORMAL_FOLLOW_ROTATE;
    chassisState.jump_ctrl = DO_NOT_JUMP;
    chassisState.fly_ctrl = NOT_FLY_MODE;

    uint8_t xyAndRefAngleMsg[8] = {0};
    uint8_t chassisStateMsg[8] = {0};

    bool forward = false;
    constexpr float YawOffsetValue1 = 5.41418511 - PiX2;
    constexpr float YawOffsetValue2 = 5.41418511 - Pi;
    ServiceMotors::Instance()->YawMotor.Offset = YawOffsetValue2;

    for (;;)
    {
        om_suber_export(remoter_suber, &remoter, false);
        if (remoter.ctrl_sw == Relax)
        {
            memset(xyAndRefAngleMsg, 0, sizeof(xyAndRefAngleMsg));
            memset(chassisStateMsg, 0, sizeof(chassisStateMsg));
            auto Vzero = static_cast<uint16_t>(2.0f * 15000.0f);
            memcpy(xyAndRefAngleMsg, &Vzero, 2);
            memcpy(xyAndRefAngleMsg + 2, &Vzero, 2);
            CAN_Transmit(&hcan2, 0xB1, xyAndRefAngleMsg, 8);
            CAN_Transmit(&hcan2, 0xB2, chassisStateMsg, 8);
        }
        else
        {
            if (remoter.ctrl_sw == Relax)
            {
                chassisState.NormalState = NONE;
                chassisState.RotateState = NORMAL_FOLLOW_ROTATE;
                chassis_msg.spinning = false;
            }
            else if (remoter.ctrl_sw == Normal)
            {
                chassisState.NormalState = NORMAL_MOVING_MODE;
                chassisState.RotateState = NORMAL_FOLLOW_ROTATE;
                chassis_msg.spinning = false;
            }
            else if (remoter.ctrl_sw == Spin)
            {
                chassisState.NormalState = NORMAL_MOVING_MODE;
                chassisState.RotateState = SPIN_ROTATE;
                chassis_msg.spinning = true;
                chassis_msg.vw = 8.0f;
            }

            uint8_t packed_mode_data =
                (chassisState.NormalState & 0x03) |
                ((chassisState.RotateState & 0x01) << 2) |
                ((chassisState.jump_ctrl & 0x03) << 3) |
                ((chassisState.fly_ctrl & 0x01) << 5);

            chassisStateMsg[0] = packed_mode_data;

            Vy_raw = forward ? -remoter.left_y : remoter.left_y;
            Vy_slope.Set_Target(Vy_raw);
            Vy_slope.Update();
            Vy_raw = Vy_slope.Get_Out();
            FloatConstrain(Vy_raw, -1.0f, 1.0f);

            Vlen_raw = remoter.left_x*0.7f;
            Vlen_slope.Set_Target(Vlen_raw);
            Vlen_slope.Update();
            Vy_raw = Vlen_slope.Get_Out();
            FloatConstrain(Vlen_raw, -2.0f, 2.0f);

            forward = (ServiceMotors::Instance()->YawMotor.Offset == YawOffsetValue1);
            float relativeAngle = ServiceMotors::Instance()->YawMotor.motorFeedback.positionFdb - ServiceMotors::Instance()->YawMotor.Offset;
            // 防止角度突变
            if (relativeAngle >= Math::Pi)
            {
                relativeAngle -= Math::PiX2;
            }
            else if (relativeAngle <= -Math::Pi)
            {
                relativeAngle += Math::PiX2;
            }

            if (chassisState.RotateState == SPIN_ROTATE)
            {
                Vy_raw = 0.0f;
                Vlen_raw = 0.0f;
                relativeAngle = 0.0f;
            }

            float distance1 = fabs(ServiceMotors::Instance()->YawMotor.motorFeedback.positionFdb - YawOffsetValue1);
            float distance2 = fabs(ServiceMotors::Instance()->YawMotor.motorFeedback.positionFdb - YawOffsetValue2);
            if (distance1 < distance2)
            {
                ServiceMotors::Instance()->YawMotor.Offset = YawOffsetValue1;
            }
            else
            {
                ServiceMotors::Instance()->YawMotor.Offset = YawOffsetValue2;
            }

            auto Vlen = static_cast<uint16_t>((Vlen_raw + 2.0f) * 15000.0f);
            auto Vy = static_cast<uint16_t>((Vy_raw + 2.0f) * 15000.0f);

            memcpy(xyAndRefAngleMsg, &Vlen, 2);
            memcpy(xyAndRefAngleMsg + 2, &Vy, 2);
            memcpy(xyAndRefAngleMsg + 4, &relativeAngle, 4);

            debug_mode = chassisStateMsg[0];

            CAN_Transmit(&hcan2, 0xB1, xyAndRefAngleMsg, 8);
            CAN_Transmit(&hcan2, 0xB2, chassisStateMsg, 8);
        }
        om_publish(chassismsg_topic, &chassis_msg, sizeof(chassis_msg), true, false);
        tx_thread_sleep(1);
    }
}
