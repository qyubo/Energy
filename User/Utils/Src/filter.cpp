//
// Created by cosmosmount on 2025/8/30.
//

#include "filter.hpp"

namespace Filter
{
    FirstOrderFilter::FirstOrderFilter()
    {
        Input = 0.0f;
        OutPut = 0.0f;
        Tau = 0.0f;
        UpdatePeriod = 0.01f; // 默认10ms
    }

    void FirstOrderFilter::SetInput(float in)
    {
        Input = in;
    }
    /*
     * @brief 设置滤波器的时间常数
     */
    void FirstOrderFilter::SetTau(float tau)
    {
        Tau = tau;
    }

    /**
     * @brief 设置滤波器的输出值
     */
    void FirstOrderFilter::SetResult(float out)
    {
        OutPut = out;
    }

    /**
     * @brief 设置滤波器的更新周期
     */
    void FirstOrderFilter::SetUpdatePeriod(float t)
    {
        UpdatePeriod = t * 0.001f;
    } /* t in ms */

    /**
     * @brief 获取滤波器的输出值
     */
    float FirstOrderFilter::GetResult()
    {
        return OutPut;
    }

    /**
     * @brief 获取滤波器的输入值
     */
    float FirstOrderFilter::GetTau()
    {
        return Tau;
    }

    /**
     * @brief 获取滤波器的更新周期
     */
    float FirstOrderFilter::GetUpdatePeriod()
    {
        return UpdatePeriod;
    }

    /**
     * @brief 初始化滤波器
     */
    void FirstOrderFilter::Init()
    {
        Clear();
        UpdatePeriod = 0.001f;
        Tau = 0.25f;
    }

    /**
     * @brief 更新滤波器
     */
    void FirstOrderFilter::Update()
    {
        float a = UpdatePeriod / (Tau);
        OutPut = (1 - a) * OutPut + a * Input;

        if (isnan(OutPut))
        {
            OutPut = Input;
        }
    }

    void FirstOrderFilter::Clear()
    {
        OutPut = 0;
        Input = 0;
    }

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
} // namespace Filter