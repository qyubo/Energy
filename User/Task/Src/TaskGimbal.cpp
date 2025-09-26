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
using namespace Math;

TX_THREAD GimbalThread;
uint8_t GimbalThreadStack[4096] = {0};

extern TX_SEMAPHORE IMUThreadSem;

#define GimbalDebug
#ifdef GimbalDebug
msg_remoter_t debug_dr16;
msg_ins_t debug_ins;
motor_debug_t yaw_debug;

pid_tuning_t yaw_pos_tuning;
pid_tuning_t yaw_spd_tuning;
pid_tuning_t pitch_pos_tuning;
pid_tuning_t pitch_spd_tuning;
#endif

double signal(double t)
{
    const double T = 0.6;           // 周期
    const double step_value = 0.52; // 最大幅值

    const double buffer_ratio = 1.0 / 5.0;
    const double rise_ratio = 4.0 / 5.0;

    double t_mod = std::fmod(t, T);

    double buffer_time = buffer_ratio * T;
    double rise_time = rise_ratio * T;

    if (t_mod < buffer_time)
    {
        // 前1/5缓冲段
        return 0.0;
    }
    else if (t_mod < T)
    {
        // 后4/5平滑上升段
        double rise_t = t_mod - buffer_time;
        double progress = rise_t / rise_time; // 0 ~ 1

        // 平滑函数：cosine ease-in
        double smooth = (1 - std::cos(Math::Pi * progress)) / 2.0;

        return step_value * smooth;
    }
    else
    {
        return 0.0;
    }
}

PID yaw_pos_pid(120.0f, 0.0f, 500.0f, 500.0f, 10.0f, PID_POSITION | PID_Derivative_On_Measurement);
PID yaw_spd_pid(100.0f, 0.0f, 2000.0f, 500.0f, 100.0f, PID_POSITION);
PID pitch_pos_pid(50.0f, 0.0f, 0.0f, 500.0f, 0.0f, PID_POSITION);
PID pitch_spd_pid(1000.0f, 0.0f, 0.0f, 12000.0f, 0.0f, PID_POSITION);

[[noreturn]] void GimbalThreadFun(ULONG initial_input) {
    UNUSED(initial_input);

    bool autoaim_on = true; // only for test, receive from operator later

    om_topic_t *gimbalctrl_topic = om_config_topic(nullptr, "ca", "gimbalctrl", sizeof(msg_gimbal_ctrl_t));
    msg_gimbal_ctrl_t gimbal_ctrl{};

    om_suber_t *remoter_suber = om_subscribe(om_find_topic("remoter", UINT32_MAX));
    om_suber_t *ins_suber = om_subscribe(om_find_topic("ins", UINT32_MAX));
    msg_remoter_t remoter{};
    msg_ins_t ins{};

    KalmanFilter YawAngleFilter;
    KalmanFilter YawGyroFilter;

    YawAngleFilter.SetQ(0.05f);
    YawAngleFilter.SetR(0.80f);
    YawGyroFilter.SetQ(0.0001f);
    YawGyroFilter.SetR(0.169f);

    // PID yaw_pos_pid(50.0f, 0.0f, 0.0f, 500.0f, 0.0f, PID_POSITION);
    // PID yaw_spd_pid(1000.0f, 0.0f, 0.0f, 12000.0f, 0.0f, PID_POSITION);
    // PID pitch_pos_pid(50.0f, 0.0f, 0.0f, 500.0f, 0.0f, PID_POSITION);
    // PID pitch_spd_pid(1000.0f, 0.0f, 0.0f, 12000.0f, 0.0f, PID_POSITION);


    for (;;) {
        om_suber_export(remoter_suber, &remoter, false);
        om_suber_export(ins_suber, &ins, false);
        memcpy (&debug_dr16, &remoter, sizeof(msg_remoter_t));
        memcpy (&debug_ins, &ins, sizeof(msg_ins_t));

        if (tx_semaphore_get(&IMUThreadSem, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            if (remoter.ctrl_sw == Relax) {
                gimbal_ctrl.pitch_torque = 0.0f;
                gimbal_ctrl.yaw_torque = 0.0f;
                gimbal_ctrl.yaw_mode = TORQUE;
                gimbal_ctrl.pitch_mode = TORQUE;
            }
            else {
                if (!autoaim_on) {
                    gimbal_ctrl.yaw_speed = remoter.right_x*3.0f;
                    gimbal_ctrl.pitch_speed = remoter.right_y;
                    gimbal_ctrl.yaw_mode = SPD;
                    gimbal_ctrl.pitch_mode = SPD;
                }
                else {
                    gimbal_ctrl.yaw_mode = TORQUE;
                    gimbal_ctrl.pitch_mode = TORQUE;

                    // yaw_pos_pid.ref += remoter.right_x*0.003f;
                    yaw_pos_pid.ref = signal(DWT_GetTimeline_s());
                    // yaw_pos_pid.fdb = YawAngleFilter.Update(ins.total_yaw*DegreeToRad);
                    yaw_pos_pid.fdb = ins.total_yaw*DegreeToRad;
                    yaw_pos_pid.UpdateResult();
                    yaw_spd_pid.ref = yaw_pos_pid.result;
                    yaw_spd_pid.fdb = YawGyroFilter.Update(ins.gyro_y);
                    yaw_spd_pid.UpdateResult();
                    gimbal_ctrl.yaw_torque = yaw_spd_pid.result;
                    //
                    // pitch_pos_pid.ref = remoter.right_y;
                    // pitch_pos_pid.fdb = ins.pitch;
                    // pitch_pos_pid.UpdateResult();
                    // pitch_spd_pid.ref = pitch_pos_pid.result;
                    // pitch_spd_pid.fdb = ins.gyro_p;
                    // pitch_spd_pid.UpdateResult();
                    // gimbal_ctrl.pitch_torque = pitch_spd_pid.result;
    #ifdef GimbalDebug
                    // yaw_pos_pid.Tuning(yaw_pos_tuning.kp, yaw_pos_tuning.ki, yaw_pos_tuning.kd);
                    // yaw_spd_pid.Tuning(yaw_spd_tuning.kp, yaw_spd_tuning.ki, yaw_spd_tuning.kd);
                    // pitch_pos_pid.Tuning(pitch_pos_tuning.kp, pitch_pos_tuning.ki, pitch_pos_tuning.kd);
                    // pitch_spd_pid.Tuning(pitch_spd_tuning.kp, pitch_spd_tuning.ki, pitch_spd_tuning.kd);
                    yaw_debug.pos_set = yaw_pos_pid.ref;
                    yaw_debug.pos_fdb = yaw_pos_pid.fdb;
                    yaw_debug.spd_set = yaw_spd_pid.ref;
                    yaw_debug.spd_fdb = yaw_spd_pid.fdb;
    #endif
                }
            }

            om_publish(gimbalctrl_topic, &gimbal_ctrl, sizeof(msg_gimbal_ctrl_t), true, false);
            tx_thread_sleep(1);
        }
    }
}