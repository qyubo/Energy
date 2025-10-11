//
// Created by cosmosmount on 2025/9/10.
//

#include "bsp_dwt.hpp"
#include "main.h"
#include "tx_api.h"

#include "om.h"
#include "pid.hpp"
#include "math.hpp"
#include "filter.hpp"
#include "magicmsgs.hpp"

using namespace Filter;
using namespace Numeric;

TX_THREAD GimbalThread;
uint8_t GimbalThreadStack[4096] = {0};

extern TX_SEMAPHORE IMUThreadSem;

#define GimbalTest
#ifdef GimbalTest
msg_remoter_t debug_dr16;
msg_ins_t debug_ins;
msg_visionrx_t debug_vrx;
motor_debug_t yaw_debug;
motor_debug_t pitch_debug;

// 200 0 0; 50 0 1500
PID yaw_pos_pid(200.0f, 0.0f, 0.0f, 500.0f, 10.0f, PID_POSITION | PID_Derivative_On_Measurement);
PID yaw_spd_pid(40.0f, 0.0f, 1800.0f, 500.0f, 100.0f, PID_POSITION);

PID pitch_pos_pid(200.0f, 0.0f, 0.0f, 500.0f, 0.0f, PID_POSITION);
PID pitch_spd_pid(200.0f, 0.0f, 1000.0f, 500.0f, 0.0f, PID_POSITION);

double yaw_signal_step = 0.82f;
double pitch_signal_T = 0.2f;

static double signal(double t)
{
    constexpr double T = 0.6;           // 周期

    constexpr double buffer_ratio = 1.0 / 5.0;
    constexpr double rise_ratio = 4.0 / 5.0;

    const double t_mod = std::fmod(t, T);

    constexpr double buffer_time = buffer_ratio * T;
    constexpr double rise_time = rise_ratio * T;

    if (t_mod < buffer_time)
    {
        // 前1/5缓冲段
        return 0.0;
    }
    else if (t_mod < T)
    {
        double step_value = yaw_signal_step;

        const double rise_t = t_mod - buffer_time;
        const double progress = rise_t / rise_time; // 0 ~ 1

        const double smooth = (1 - std::cos(Numeric::Pi * progress)) / 2.0;

        return step_value * smooth;
    }
    else
    {
        return 0.0;
    }
}

static double tri_signal(double t)
{
    constexpr double T = 0.6;           // 周期

    constexpr double buffer_ratio = 1.0 / 5.0;
    constexpr double rise_ratio = 4.0 / 5.0;

    const double t_mod = std::fmod(t, T);

    constexpr double buffer_time = buffer_ratio * T;
    constexpr double rise_time = rise_ratio * T;

    if (t_mod < buffer_time)
    {
        // 前1/5缓冲段
        return 0.0;
    }
    else if (t_mod < T)
    {
        double step_value = yaw_signal_step;
        // 后4/5 线性上升段（三角波）
        const double rise_t = t_mod - buffer_time;
        const double progress = rise_t / rise_time; // 0 ~ 1

        // 线性
        const double smooth = progress;

        return step_value * smooth;
    }
    else
    {
        return 0.0;
    }
}

static double sin_signal(double t)
{
    const double amplitude = 0.03;  // 振幅（小幅度）
    double T = pitch_signal_T;          // 周期（快速振动）
    const double omega = 2.0 * M_PI / T; // 角频率 ω = 2π / T

    return amplitude * std::sin(omega * t);
}

#endif

[[noreturn]] void GimbalThreadFun(ULONG initial_input)
{
    UNUSED(initial_input);

    bool autoaim_on = true; // only for test, receive from operator later

    om_init();

    om_topic_t *gimbalctrl_topic = om_config_topic(nullptr, "ca", "gimbalctrl", sizeof(msg_gimbal_ctrl_t));
    msg_gimbal_ctrl_t gimbal_ctrl{};
    om_suber_t *remoter_suber = om_subscribe(om_find_topic("remoter", UINT32_MAX));
    msg_remoter_t remoter{};
    om_suber_t *ins_suber = om_subscribe(om_find_topic("ins", UINT32_MAX));
    msg_ins_t ins{};
    om_suber_t *chassis_suber = om_subscribe(om_find_topic("chassis", UINT32_MAX));
    msg_chassis_t chassis{};
    om_suber_t *vision_suber = om_subscribe(om_find_topic("visionrx", UINT32_MAX));
    msg_visionrx_t vision_rx{};

    // KalmanFilter YawAngleFilter;
    KalmanFilter YawGyroFilter;
    //
    // YawAngleFilter.SetQ(0.05f);
    // YawAngleFilter.SetR(0.80f);
    YawGyroFilter.SetQ(0.0001f);
    YawGyroFilter.SetR(0.169f);

#ifdef GimbalTest
    float yaw_init = 0.0f;
    float pitch_init = 0.0f;
#else
    PID yaw_pos_pid(100.0f, 0.0f, 0.0f, 500.0f, 10.0f, PID_POSITION | PID_Derivative_On_Measurement);
    PID yaw_spd_pid(50.0f, 0.0f, 500.0f, 500.0f, 100.0f, PID_POSITION);

    PID pitch_pos_pid(50.0f, 0.0f, 0.0f, 500.0f, 0.0f, PID_POSITION);
    PID pitch_spd_pid(10.0f, 0.0f, 0.0f, 500.0f, 0.0f, PID_POSITION);
#endif


    for (;;) {
        om_suber_export(remoter_suber, &remoter, false);
        om_suber_export(ins_suber, &ins, false);
        om_suber_export(chassis_suber, &chassis, false);
        om_suber_export(vision_suber, &vision_rx, false);
        memcpy (&debug_dr16, &remoter, sizeof(msg_remoter_t));
        memcpy (&debug_ins, &ins, sizeof(msg_ins_t));
        memcpy (&debug_vrx, &vision_rx, sizeof(msg_visionrx_t));



        if (tx_semaphore_get(&IMUThreadSem, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            if (remoter.ctrl_sw == Relax || remoter.offline)
            {
                gimbal_ctrl.pitch_torque = 0.0f;
                gimbal_ctrl.yaw_torque = 0.0f;
                gimbal_ctrl.yaw_mode = TORQUE;
                gimbal_ctrl.pitch_mode = TORQUE;
            }
            else
            {
                // if (!autoaim_on)
                if (remoter.ctrl_sw == Normal)
                {
                    // gimbal_ctrl.yaw_speed = remoter.right_x*7.0f;
                    // gimbal_ctrl.pitch_speed = remoter.right_y*7.0f;
                    // gimbal_ctrl.yaw_mode = SPD;
                    // gimbal_ctrl.pitch_mode = SPD;
                    gimbal_ctrl.yaw_mode = TORQUE;
                    gimbal_ctrl.pitch_mode = TORQUE;
                    yaw_pos_pid.ref = ins.total_yaw*DegreeToRad + remoter.right_x*0.03f;
                    pitch_pos_pid.ref = ins.pitch*DegreeToRad + remoter.right_y*0.03f;

                    yaw_pos_pid.fdb = ins.total_yaw*DegreeToRad;
                    yaw_pos_pid.UpdateResult();

                    yaw_spd_pid.ref = yaw_pos_pid.result;
                    if (chassis.spinning)
                    {
                        yaw_spd_pid.ref -= chassis.vw;
                    }
                    yaw_spd_pid.fdb = YawGyroFilter.Update(ins.gyro_y);
                    yaw_spd_pid.UpdateResult();
                    gimbal_ctrl.yaw_torque = yaw_spd_pid.result;

                    pitch_pos_pid.ref = FloatConstrain(pitch_pos_pid.ref, -25.0f*DegreeToRad, 15.0f*DegreeToRad);
                    pitch_pos_pid.fdb = ins.pitch*DegreeToRad;
                    pitch_pos_pid.UpdateResult();
                    pitch_spd_pid.ref = pitch_pos_pid.result;
                    pitch_spd_pid.fdb = ins.gyro_p;
                    pitch_spd_pid.UpdateResult();
                    gimbal_ctrl.pitch_torque = pitch_spd_pid.result;
                }
                // else
                if (remoter.ctrl_sw == Spin)
                {
                    gimbal_ctrl.yaw_mode = TORQUE;
                    gimbal_ctrl.pitch_mode = TORQUE;
#ifdef GimbalTest
                    if (yaw_init == 0.0f)
                        yaw_init = ins.total_yaw*DegreeToRad;
                    // yaw_pos_pid.ref = ins.total_yaw*DegreeToRad + remoter.right_x*0.03f;
                    // yaw_pos_pid.ref = yaw_init + tri_signal(DWT_GetTimeline_s());
                    yaw_pos_pid.ref = yaw_init + signal(DWT_GetTimeline_s());
                    // yaw_pos_pid.ref = yaw_init;
                    pitch_pos_pid.ref = ins.pitch*DegreeToRad + remoter.right_y*0.03f;
                    if (pitch_init == 0.0f)
                        pitch_init = ins.pitch*DegreeToRad;
                    // pitch_pos_pid.ref = -0.1f + sin_signal(DWT_GetTimeline_s());
#endif

                    yaw_pos_pid.fdb = ins.total_yaw*DegreeToRad;
                    yaw_pos_pid.UpdateResult();

                    yaw_spd_pid.ref = yaw_pos_pid.result;
                    if (chassis.spinning)
                    {
                        yaw_spd_pid.ref -= chassis.vw;
                    }
                    yaw_spd_pid.fdb = YawGyroFilter.Update(ins.gyro_y);
                    yaw_spd_pid.UpdateResult();
                    gimbal_ctrl.yaw_torque = yaw_spd_pid.result;

                    pitch_pos_pid.ref = FloatConstrain(pitch_pos_pid.ref, -25.0f*DegreeToRad, 15.0f*DegreeToRad);
                    pitch_pos_pid.fdb = ins.pitch*DegreeToRad;
                    pitch_pos_pid.UpdateResult();
                    pitch_spd_pid.ref = pitch_pos_pid.result;
                    pitch_spd_pid.fdb = ins.gyro_p;
                    pitch_spd_pid.UpdateResult();
                    gimbal_ctrl.pitch_torque = pitch_spd_pid.result;
    #ifdef GimbalTest
                    // yaw_pos_pid.Tuning(yaw_pos_tuning.kp, yaw_pos_tuning.ki, yaw_pos_tuning.kd);
                    // yaw_spd_pid.Tuning(yaw_spd_tuning.kp, yaw_spd_tuning.ki, yaw_spd_tuning.kd);
                    // pitch_pos_pid.Tuning(pitch_pos_tuning.kp, pitch_pos_tuning.ki, pitch_pos_tuning.kd);
                    // pitch_spd_pid.Tuning(pitch_spd_tuning.kp, pitch_spd_tuning.ki, pitch_spd_tuning.kd);
                    yaw_debug.pos_set = yaw_pos_pid.ref;
                    yaw_debug.pos_fdb = yaw_pos_pid.fdb;
                    yaw_debug.spd_set = yaw_spd_pid.ref;
                    yaw_debug.spd_fdb = yaw_spd_pid.fdb;

                    pitch_debug.pos_set = pitch_pos_pid.ref;
                    pitch_debug.pos_fdb = pitch_pos_pid.fdb;
                    pitch_debug.spd_set = pitch_spd_pid.ref;
                    pitch_debug.spd_fdb = pitch_spd_pid.fdb;
    #endif
                }
            }

            om_publish(gimbalctrl_topic, &gimbal_ctrl, sizeof(msg_gimbal_ctrl_t), true, false);
            tx_thread_sleep(1);
        }
    }
}