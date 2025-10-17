//
// Created by cosmosmount on 2025/8/30.
//

#include "filter.hpp"

using namespace Numeric;

namespace Filter
{
    KalmanFilter::KalmanFilter()
    {
        LastP = 0.02f;
        NowP = 0.0f;
        result = 0.0f;
        Kg = 0.0f;
        Q = 0.001f;
        R = 0.543f;
    }

    void KalmanFilter::Clear()
    {
        LastP = 0.02f;
        NowP = 0.0f;
        result = 0.0f;
        Kg = 0.0f;
        Q = 0.001f;
        R = 0.543f;
    }

    void KalmanFilter::SetKg(float kg)
    {
        this->Kg = kg;
    }

    void KalmanFilter::SetQ(float q)
    {
        this->Q = q;
    }

    void KalmanFilter::SetR(float r)
    {
        this->R = r;
    }

    float KalmanFilter::Update(float input)
    {
        // 预测协方差方程：k时刻系统估算协方差 = k-1时刻的系统协方差 + 过程噪声协方差
        this->NowP = this->LastP + this->Q;

        // 卡尔曼增益方程：卡尔曼增益 = k1-1时刻系统估算协方差 / （k时刻系统估算协方差 + 观测噪声协方差）
        this->Kg = this->NowP * (1.0f / (this->NowP + this->R));
        // 更新最优值方程：k时刻状态变量的最优值 = 状态变量的预测值 + 卡尔曼增益 * （测量值 - 状态变量的预测值）
        this->result = this->result + this->Kg * (input - this->result); // 因为这一次的预测值就是上一次的输出值
        // 更新协方差方程: 本次的系统协方差付给 kfp->LastP 威下一次运算准备。
        this->LastP = (1.0f - this->Kg) * this->NowP;

        return this->result;
    }

    IIRFilter::IIRFilter(uint32_t _order, Filter_Mode _mode, int _freq_low, int _freq_high)
    {
        num_stage = _order / 2;
        if (_mode == LOWPASS)
        {
            switch (_freq_low)
            {
            case 2:
                coeff = {
                    //b0  b1    b2    a1                                          a2
                    1.0f, 2.0f, 1.0f, 1.982228929792528626663283830566797405481f, -0.982385450614125299573231586691690608859f
                };
                gain = 0.000039130205399144361486617194056947255f;
            case 333:
                coeff = {
                    //b0  b1    b2    a1                                           a2
                    1.0f, 2.0f, 1.0f, -0.617669743139197424675046477204887196422f, -0.239839843702840921357832826288358774036f
                };
                gain = 0.464377396710509593447113729780539870262f;
            }
        }
        arm_biquad_cascade_df1_init_f32(&section, num_stage, coeff.data(), buff);
    }

    float IIRFilter::Update(float _input) const
    {
        float result;
        arm_biquad_cascade_df1_f32(&section, &_input, &result, 1);
        result *= gain;
        return result;
    }


    FIRFilter::FIRFilter(uint32_t _order, float _constrain_low, float _constrain_high,
        Filter_Mode _mode, float _freq_low, float _freq_high)
    {
        order = _order;
        constrain_low = _constrain_low;
        constrain_high = _constrain_high;
        filter_mode = _mode;
        freq_low = _freq_low;
        freq_high = _freq_high;

        // 将所有计算所得值进行softmax操作成和为1的值
        float system_function_sum = 0.0f;
        // 特征低角速度
        float omega_low = 2.0f * PI * freq_low / fs;
        // 特征高角速度
        float omega_high = 2.0f * PI * freq_high / fs;

        // 计算滤波器系统

        switch (filter_mode)
        {
        case LOWPASS:
        {
            for (int i = 0; i < order + 1; i++)
            {
                system_function[i] = omega_low / PI * arm_sin_f32((static_cast<float>(i) - order / 2.0f) * omega_low);
            }

            break;
        }
        case HIGHPASS:
        {
            for (int i = 0; i < order + 1; i++)
            {
                system_function[i] = arm_sin_f32((static_cast<float>(i) - order / 2.0f) * PI)
                                    - omega_high / PI * arm_sin_f32((static_cast<float>(i) - order / 2.0f) * omega_high);
            }

            break;
        }
        case BANDPASS:
        {
            for (int i = 0; i < order + 1; i++)
            {
                system_function[i] = omega_high / PI * arm_sin_f32((static_cast<float>(i) - order / 2.0f) * omega_high)
                                    - omega_low / PI * arm_sin_f32((static_cast<float>(i) - order / 2.0f) * omega_low);
            }

            break;
        }
        case BANDSTOP:
        {
            for (int i = 0; i < order + 1; i++)
            {
                system_function[i] = arm_sin_f32((static_cast<float>(i) - order / 2.0f) * PI)
                        + omega_low / PI * arm_sin_f32((static_cast<float>(i)- order / 2.0f) * omega_low)
                        - omega_high / PI * arm_sin_f32((static_cast<float>(i) - order / 2.0f) * omega_high);
            }

            break;
        }
        }

        for (int i = 0; i < order + 1; i++)
        {
            system_function_sum += system_function[i];
        }

        for (int i = 0; i < order + 1; i++)
        {
            system_function[i] /= system_function_sum;
        }
    }

    void FIRFilter::SetNow(const float _now)
    {
        float now_value;

        // 输入限幅, 全0为不限制
        if (constrain_low != 0.0f || constrain_high != 0.0f)
            now_value = FloatConstrain(_now, constrain_low, constrain_high);
        else
            now_value = _now;

        // 将当前值放入被卷积的信号中
        input_signal[signal_flag] = now_value;
        signal_flag++;

        // 若越界则轮回
        if (signal_flag == order + 1)
        {
            signal_flag = 0;
        }
    }

    float FIRFilter::Update() const
    {
        float result = 0.0f;
        // 执行卷积操作
        for (int i = 0; i < order + 1; i++)
        {
            result += system_function[i] * input_signal[(signal_flag + i) % (order + 1)];
        }

        return result;
    }

} // namespace Filter