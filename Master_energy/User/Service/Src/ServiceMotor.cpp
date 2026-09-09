// #include "main.h"

// #include "ServiceMotor.hpp"
// #include "om.h"
// #include "magicmsgs.hpp"
// #include "math.hpp"
// #include "bsp_dwt.hpp"

// using namespace Numeric;

// extern CAN_HandleTypeDef hcan1;
// extern CAN_HandleTypeDef hcan2;
// extern CAN_HandleTypeDef hcan3;

// DJIMotorHandler* DJIMotorhandler = DJIMotorHandler::Instance();

// pid_tuning_t motor_pos_pid;
// pid_tuning_t motor_spd_pid;
// float debug_cur = 0.0f;
// float vel_ratio = 0;
// float test_r = 0;
// float debug_set = 0.0f;

// void ServiceMotors::MotorRegister()
// {
//     // //注册电机
//     DJIMotorhandler->registerMotor(&FuMotor, &hcan1, 0x206);
//     FuMotor.currentSet = 0;
//     FuMotor.gearBox = GearBox_None;
// }

// void ServiceMotors::SetModeAndPidParam()
// {
//     FuMotor.speedPid.kp = 100.0f;
// }

