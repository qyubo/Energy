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

    om_topic_t *gimbalctrl_topic = om_config_topic(nullptr, "ca", "gimbalctrl", sizeof(msg_gimbal_ctrl_t));
    msg_gimbal_ctrl_t gimbal_ctrl{};
    om_suber_t *remoter_suber = om_subscribe(om_find_topic("remoter", UINT32_MAX));
    msg_remoter_t remoter{};
    om_suber_t *ins_suber = om_subscribe(om_find_topic("ins", UINT32_MAX));
    msg_ins_t ins{};
    // om_suber_t *vision_suber = om_subscribe(om_find_topic("visionrx", UINT32_MAX));
    // msg_visionrx_t vision_rx{};


    for (;;) {
        om_suber_export(remoter_suber, &remoter, false);
        om_suber_export(ins_suber, &ins, false);
        // om_suber_export(vision_suber, &vision_rx, false);
        memcpy (&debug_dr16, &remoter, sizeof(msg_remoter_t));
        memcpy (&debug_ins, &ins, sizeof(msg_ins_t));
        // memcpy (&debug_vrx, &vision_rx, sizeof(msg_visionrx_t));

        if (tx_semaphore_get(&IMUThreadSem, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            om_publish(gimbalctrl_topic, &gimbal_ctrl, sizeof(msg_gimbal_ctrl_t), true, false);
            tx_thread_sleep(1);
        }
    }
}