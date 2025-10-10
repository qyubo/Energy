/**
 * @file Slope.cpp
 * @author yssickjgd (1345578933@qq.com)
 * @modified
 * @brief 斜坡函数, 用于速度规划等
 * @version 0.1
 * @date 2023-08-29 0.1 23赛季定稿
 * @date 2024-06-03 1.1 规划引入优先级方式
 * @date 2025-03-01 PnX更改与部署
 * @copyright USTC-RoboWalker (c) 2023-2024
 *
 */
#include "slope.hpp"

void SLOPE::Init(float __Increase_Value, float __Decrease_Value, PriorType __Slope_Prior)
{
    Increase_Value = __Increase_Value;
    Decrease_Value = __Decrease_Value;
    Slope_Prior = __Slope_Prior;
}

void SLOPE::Update()
{
    // 规划为当前真实值优先的额外逻辑
    if (Slope_Prior == REAL_Prior)
    {
        if ((Target >= Now_Real && Now_Real >= Now_Planning) || (Target <= Now_Real && Now_Real <= Now_Planning))
        {
            Out = Now_Real;
        }
    }

    if (Now_Planning > 0.0f)
    {
        if (Target > Now_Planning)
        {
            // 正值加速
            if (Numeric::abs(Now_Planning - Target) > Increase_Value)
            {
                Out += Increase_Value;
            }
            else
            {
                Out = Target;
            }
        }
        else if (Target < Now_Planning)
        {
            // 正值减速
            if (Numeric::abs(Now_Planning - Target) > Decrease_Value)
            {
                Out -= Decrease_Value;
            }
            else
            {
                Out = Target;
            }
        }
    }
    else if (Now_Planning < 0.0f)
    {
        if (Target < Now_Planning)
        {
            // 负值加速
            if (Numeric::abs(Now_Planning - Target) > Increase_Value)
            {
                Out -= Increase_Value;
            }
            else
            {
                Out = Target;
            }
        }
        else if (Target > Now_Planning)
        {
            // 负值减速
            if (Numeric::abs(Now_Planning - Target) > Decrease_Value)
            {
                Out += Decrease_Value;
            }
            else
            {
                Out = Target;
            }
        }
    }
    else
    {
        if (Target > Now_Planning)
        {
            // 0值正加速
            if (Numeric::abs(Now_Planning - Target) > Increase_Value)
            {
                Out += Increase_Value;
            }
            else
            {
                Out = Target;
            }
        }
        else if (Target < Now_Planning)
        {
            // 0值负加速
            if (Numeric::abs(Now_Planning - Target) > Increase_Value)
            {
                Out -= Increase_Value;
            }
            else
            {
                Out = Target;
            }
        }
    }

    // 善后工作
    Now_Planning = Out;
}
