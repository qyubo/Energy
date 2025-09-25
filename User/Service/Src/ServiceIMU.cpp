//
// Created by cosmosmount on 2025/9/2.
//

#include "ServiceIMU.hpp"
#include "magicmsgs.hpp"

using namespace BMI088;
using namespace AHRS;

cBMI088 bmi088;
cIMU *imu_handler = &bmi088;

TX_THREAD IMUThread;
uint8_t IMUThreadStack[4096] = {0};
TX_SEMAPHORE IMUThreadSem;
ULONG IMU_time;

static float last_yaw = 0.0f;
static float yaw_corr = 0.0f;
const float alpha = 1.0f;
static bool corr_start = false;
static bool yaw_static = false;

static void InitQuaternion(float *init_q4)
{
    float acc_init[3] = {0};
    float gravity_norm[3] = {0, 0, 1}; // 导航系重力加速度矢量,归一化后为(0,0,1)
    float axis_rot[3] = {0};           // 旋转轴
    // 读取100次加速度计数据,取平均值作为初始值
    for (uint8_t i = 0; i < 100; ++i)
    {
        acc_init[X] += imu_handler->acc_data.x;
        acc_init[Y] += imu_handler->acc_data.y;
        acc_init[Z] += imu_handler->acc_data.z;
        DWT_Delay(0.001);
    }
    for (uint8_t i = 0; i < 3; ++i)
        acc_init[i] /= 100;
    Math::Norm3d(acc_init);
    // 计算原始加速度矢量和导航系重力加速度矢量的夹角
    float angle = acosf(Math::Dot3d(acc_init, gravity_norm));
    Math::Cross3d(acc_init, gravity_norm, axis_rot);
    Math::Norm3d(axis_rot);
    init_q4[0] = cosf(angle / 2.0f);
    for (uint8_t i = 0; i < 2; ++i)
        init_q4[i + 1] = axis_rot[i] * sinf(angle / 2.0f); // 轴角公式,第三轴为0(没有z轴分量)
}

[[noreturn]] void IMUThreadFun(ULONG initial_input) {
    UNUSED(initial_input);

    /* INS Topic */
    om_topic_t *ins_topic = om_config_topic(nullptr, "ca", "ins", sizeof(msg_ins_t));
    msg_ins_t msg_ins{};

    imu_handler->self_test.ACC_CHIP_ID_ERR = true;       // 加速度计ID错误则为true
    imu_handler->self_test.ACC_DATA_ERR = true;          // 加速度计数据错误则为true
    imu_handler->self_test.GYRO_CHIP_ID_ERR = true;      // 陀螺仪ID错误则为true
    imu_handler->self_test.GYRO_DATA_ERR = true;         // 陀螺仪数据错误则为true
    imu_handler->self_test.INIT_ERR = true;       // BMI088初始化错误则为true
    imu_handler->self_test.CALIBRATE_ERR = false; // BMI088标定错误则为true
    imu_handler->self_test.TEMP_CTRL_ERR = false; // BMI088温度控制错误则为true

    imu_handler->Config(); //< 初始化配置

    imu_handler->VerifyAccChipID();  //< 验证加速度计ID
    imu_handler->VerifyGyroChipID(); //< 验证陀螺仪ID

    while (imu_handler->acc_data.temperature < 45.0f) {
        tx_thread_sleep(50);
    }
    tx_thread_sleep(1000);

    imu_handler->Calibrate(); //< 标定IMU
    imu_handler->self_test.INIT_ERR = false;

    uint32_t INS_Count = 0;
    float init_quaternion[4] = {0};
    InitQuaternion(init_quaternion);
    IMU_QuaternionEKF_Init(init_quaternion, 10, 0.001, 1000000, 1, 0);
    DWT_GetDeltaT(&INS_Count);

    for (;;) {
        IMU_time = tx_time_get();
        if (!imu_handler->self_test.INIT_ERR) {
            imu_handler->ReadAccData(&imu_handler->acc_data);
            imu_handler->ReadGyroData(&imu_handler->gyro_data);
            imu_handler->gyro_data.roll -= BMI088_PRE_CALI_ACC_X_OFFSET;
            imu_handler->gyro_data.pitch -= BMI088_PRE_CALI_ACC_Y_OFFSET;
            imu_handler->gyro_data.yaw -= BMI088_PRE_CALI_ACC_Z_OFFSET;
            // if ((int)DWT_GetTimeline_ms() % 5 == 0)
            // {
            //     if (fabs(imu_handler->acc_data.x) <= 0.3f && fabs(imu_handler->acc_data.y) <= 0.3f &&
            //     fabs(imu_handler->gyro_data.roll) <= 0.03f && fabs(imu_handler->gyro_data.pitch) <= 0.03f && fabs(imu_handler->gyro_data.yaw) <= 0.04f)
            //     {
            //         if (!corr_start)
            //         {
            //             yaw_corr = QEKF_INS.Yaw;
            //             corr_start = true;
            //         }
            //         yaw_static = true;
            //
            //     }
            //     else
            //     {
            //         corr_start = false;
            //         yaw_static = false;
            //     }
            // }
            // if (yaw_static)
            // {
            //     imu_handler->gyro_data.yaw -= (QEKF_INS.Yaw-yaw_corr);
            // }
            // if (fabs(imu_handler->acc_data.x) <= 0.3f && fabs(imu_handler->acc_data.y) <= 0.3f &&
            //     fabs(imu_handler->gyro_data.roll) <= 0.03f && fabs(imu_handler->gyro_data.pitch) <= 0.03f && fabs(imu_handler->gyro_data.yaw) <= 0.04f)
            // {
            //     imu_handler->gyro_data.yaw = 0.0f;
            // }
            if (fabs(imu_handler->acc_data.x) <= 0.15f && fabs(imu_handler->acc_data.y) <= 0.15f &&
                fabs(imu_handler->gyro_data.roll) <= 0.015f && fabs(imu_handler->gyro_data.pitch) <= 0.015f && fabs(imu_handler->gyro_data.yaw) <= 0.015f)
            {
                imu_handler->gyro_data.yaw = 0;
            }
            IMU_QuaternionEKF_Update(imu_handler->gyro_data.roll, imu_handler->gyro_data.pitch, imu_handler->gyro_data.yaw,
                imu_handler->acc_data.x, imu_handler->acc_data.y, imu_handler->acc_data.z,
                DWT_GetDeltaT(&INS_Count));
        }

        tx_semaphore_put(&IMUThreadSem);

        memcpy(msg_ins.quaternion, QEKF_INS.q, sizeof(QEKF_INS.q));
        msg_ins.yaw = QEKF_INS.Yaw;
        msg_ins.pitch = QEKF_INS.Pitch;
        msg_ins.roll = QEKF_INS.Roll;
        msg_ins.total_yaw = QEKF_INS.YawTotalAngle;
        msg_ins.gyro_r = imu_handler->gyro_data.roll;
        msg_ins.gyro_p = imu_handler->gyro_data.pitch;
        msg_ins.gyro_y = imu_handler->gyro_data.yaw;

        om_publish(ins_topic, &msg_ins, sizeof(msg_ins), true, false);

        uint8_t time_to_delay = tx_time_get() - IMU_time;
        if (time_to_delay < 1) {
            tx_thread_sleep(1 - time_to_delay);
        }
    }
}

TX_THREAD IMUTempThread;
uint8_t IMUTempThreadStack[2048] = {0};
// TX_SEMAPHORE IMUTempThreadSem;
ULONG temp_time;

[[noreturn]] void IMUTempThreadFun(ULONG initial_input) {
    UNUSED(initial_input);

    imu_handler->TempPid = PID(650.0f, 0.06f, 10.0f, 900.0f, 300.0f,  PID_POSITION | PID_Integral_Limit | PID_Changing_Integral_Rate | PID_Derivative_On_Measurement);
    imu_handler->TempPid.ScalarA = 3.5f;
    imu_handler->TempPid.ScalarB = 0.08f;

    imu_handler->TempFdbFilter.SetTau(0.1f);       // 设置滤波时间常数
    imu_handler->TempFdbFilter.SetUpdatePeriod(1); // 设置更新周期

    imu_handler->TargetTemp = 45.0f;                              //< 设置目标温度，一般为40度以上
    PWM_Start(&HEATING_RESISTANCE_TIM, TIM_CHANNEL_1); //< 启动加热电阻PWM

    float tmp_last = imu_handler->acc_data.temperature;
    tx_thread_sleep(1000);
    imu_handler->ReadAccTemperature(&imu_handler->acc_data.temperature);
    if (tmp_last == imu_handler->acc_data.temperature) {
        //error in temp
        tx_thread_suspend(&IMUTempThread);
    }

    for (;;) {
        temp_time = tx_time_get();
        imu_handler->ReadAccTemperature(&imu_handler->acc_data.temperature);
        imu_handler->TemperatureControl(imu_handler->TargetTemp);

        uint8_t time_to_delay = tx_time_get() - temp_time;
        if (time_to_delay < 1) {
            tx_thread_sleep(1 - time_to_delay);
        }
    }
}
