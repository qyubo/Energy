//
// Created by cosmosmount on 2025/8/29.
//

#ifndef RM26_FILTER_HPP
#define RM26_FILTER_HPP

#include <stdint.h>
#include <math.h>
#include "arm_math.h"
#include "math.hpp"

namespace Filter
{
    class FirstOrderFilter
    {
    private:
        float Input;        // 设置滤波器的输入值
        float OutPut;       // 设置滤波器的输出值
        float Tau;          // 设置滤波器的时间常数
        float UpdatePeriod; // 设置滤波器的更新周期，单位为秒，但是设置时以毫秒为单位

    public:
        /**
         * @brief 构造函数，简单复制初始化，避免出现未知错误
         */
        FirstOrderFilter();

        /**
     * @brief 设置滤波器的输入值
     */
        void SetInput(float in);

        /*
         * @brief 设置滤波器的时间常数
         */
        void SetTau(float tau);

        /**
         * @brief 设置滤波器的输出值
         */
        void SetResult(float out);

        /**
         * @brief 设置滤波器的更新周期
         */
        void SetUpdatePeriod(float t);

        /**
         * @brief 获取滤波器的输出值
         */
        float GetResult();

        /**
         * @brief 获取滤波器的输入值
         */
        float GetTau();

        /**
         * @brief 获取滤波器的更新周期
         */
        float GetUpdatePeriod();

        /**
         * @brief 初始化滤波器
         */
        void Init();

        /**
         * @brief 更新滤波器
         */
        void Update();

        void Clear();
    };

    class KalmanFilter
    {
        float LastP; // 上次估算协方差 初始化值为0.02		--e(ESTk-1)  上次协方差
        float NowP; // 当前估算协方差 初始化值为0		--预测e(ESTk)	当前估算协方差
        float result;   // 卡尔曼滤波器输出 初始化值为0
        float Kg;    // 卡尔曼增益 初始化值为0				--Kk
        float Q;     // 过程噪声协方差 初始化值为0.001
        float R;     // 观测噪声协方差 初始化值为0.543		--e(MEAk)  测量误差
    public:
        /**
         * @brief 构造函数，简单复制初始化，避免出现未知错误
         */
        KalmanFilter();

        /**
         * @brief 清空卡尔曼滤波器
         */
        void Clear();

        /**
         * @brief 设置卡尔曼滤波器的增益
         * @param kg 卡尔曼滤波器的增益
         * @return void
         */
        void SetKg(float kg);

        /**
         * @brief 设置卡尔曼滤波器的过程噪声协方差
         * @param q 过程噪声协方差
         */
        void SetQ(float q);

        /**
         * @brief 设置卡尔曼滤波器的观测噪声协方差
         */
        void SetR(float r);

        /**
         * @brief 更新卡尔曼滤波器的输出
         * @param input 卡尔曼滤波器的输入
         * @return 卡尔曼滤波器的输出
         */
        float Update(float input);
    };

    class LowPassFilter_333Hz
    {
    private:
        float Output;
        float buff[8] = {0};
        float coeff[5] = {
            //b0  b1    b2    a1                                           a2
            1.0f, 2.0f, 1.0f, -0.617669743139197424675046477204887196422f, -0.239839843702840921357832826288358774036f
        };  //滤波器系数，MATLAB生成去掉a0,a1a2取反
        float gain = 0.464377396710509593447113729780539870262f;//x阶IIR滤波器，gain需要乘以（x/2）次方
        arm_biquad_casd_df1_inst_f32 S;

    public:

        LowPassFilter_333Hz(){
            arm_biquad_cascade_df1_init_f32(&S, 1, coeff, buff);
        }

        float calculate(float _input)
        {
            float temp;
            arm_biquad_cascade_df1_f32(&S, &_input, &temp, 1);
            temp *= gain;
            Output = temp;
            return Output;
        }

        void reset(){
            for (auto& i : buff){
                i = 0;
            }
        }

        float getOutput(){
            return Output;
        }

    };




}

#endif //RM26_FILTER_HPP