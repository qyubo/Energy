/**
* @file Slope.hpp
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
#ifndef SLOPE_HPP
#define SLOPE_HPP

#include "math.hpp"

enum PriorType
{
    REAL_Prior = 0,
    TARGET_Prior = 1,
};

class SLOPE
{
public:
    void Init(float __Increase_Value, float __Decrease_Value, PriorType __Slope_Prior = REAL_Prior);

    void Reset(float __Increase_Value, float __Decrease_Value, PriorType __Slope_Prior = REAL_Prior);

    void Update();

    inline float Get_Out();

    inline void Set_Now_Real(float __Now_Real);

    inline void Set_Increase_Value(float __Increase_Value);

    inline void Set_Decrease_Value(float __Decrease_Value);

    inline void Set_Target(float __Target);
protected:
    float Out = 0.0f;
    float Now_Real = 0.0f;
    float Now_Planning = 0.0f;
    float Increase_Value = 0.0f;
    float Decrease_Value = 0.0f;
    float Target = 0.0f;
    PriorType Slope_Prior = REAL_Prior;
};

inline float SLOPE::Get_Out()
{
    return Out;
}

inline void SLOPE::Set_Now_Real(float __Now_Real)
{
    Now_Real = __Now_Real;
}

inline void SLOPE::Set_Increase_Value(float __Increase_Value)
{
    Increase_Value = __Increase_Value;
}

inline void SLOPE::Set_Decrease_Value(float __Decrease_Value)
{
    Decrease_Value = __Decrease_Value;
}

inline void SLOPE::Set_Target(float __Target)
{
    Target = __Target;
}

#endif