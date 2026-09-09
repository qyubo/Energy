//
// Created by cosmosmount on 2025/8/30.
//

#include "math.hpp"

namespace Numeric
{
    float LoopFloatConstrain(float input, float minValue, float maxValue)
    {
        if (maxValue < minValue)
            return input; // 保持原样，因为输入的范围不合逻辑

        const float range = maxValue - minValue;

        if (range == 0.0f)
            return minValue; // 最大值和最小值相等时直接返回任一值

        float normalizedInput = input - minValue;

        normalizedInput = fmod(normalizedInput, range); // 使用模运算确保结果在[minValue, maxValue]之间

        if (normalizedInput < 0)
            normalizedInput += range; // 确保结果非负

        return minValue + normalizedInput;
    }

    float FloatConstrain(float Input, float minValue, float maxValue)
    {
        if (maxValue < minValue)
            return Input;

        if (Input > maxValue)
            Input = maxValue;

        else if (Input < minValue)
            Input = minValue;

        return Input;
    }

    float LimitABS(float _input, float _max)
    {
        if (_input > _max)
        {
            return _max;
        }
        else if (_input < -_max)
        {
            return -_max;
        }
        return _input;
    }

    float LimitABS_SMALLER(float _input, float _max, float _constraint)
    {
        if (_input > _max)
        {
            return _constraint;
        }
        else if (_input < -_max)
        {
            return -_constraint;
        }
        return _input;
    }


    uint32_t ConvertToFixed(float _inNum, float _inMin, float _inPrecision)
    {
        return (uint32_t)((_inNum - _inMin) / _inPrecision);
    }

    float ConvertFromFixed(uint32_t _inNum, float _inMin, float _inPrecision)
    {
        return (float)(_inNum)*_inPrecision + _inMin;
    }

    float invSqrt(float x)
    {
        float xhalf = 0.5f * x;
        int i = *(int *)&x;             // 获取浮点值的位表示
        i = 0x5f3759df - (i >> 1);      // 初始猜测
        x = *(float *)&i;               // 将位转换回浮点数
        x = x * (1.5f - xhalf * x * x); // 牛顿迭代步骤
        return 1.0f / x;
    }

    float Sqrt(float x)
    {
        float y;
        float delta;
        float maxError;

        if (x <= 0)
        {
            return 0;
        }

        // initial guess
        y = x / 2;

        // refine
        maxError = x * 0.001f;

        do
        {
            delta = (y * y) - x;
            y -= delta / (2 * y);
        } while (delta > maxError || delta < -maxError);

        return y;
    }

    float abs(float x)
    {
        return x > 0.0f ? x : -x;
    }

    float sign(float value)
    {
        if (value >= 0.0f)
        {
            return 1.0f;
        }
        else
        {
            return -1.0f;
        }
    }

    float FloatDeadband(float Value, float minValue, float maxValue)
    {
        if (Value < maxValue && Value > minValue)
        {
            Value = 0.0f;
        }
        return Value;
    }

    int16_t Int16Constrain(int16_t Value, int16_t minValue, int16_t maxValue)
    {
        if (Value < minValue)
            return minValue;
        else if (Value > maxValue)
            return maxValue;
        else
            return Value;
    }

    float ThetaFormat(float Ang)
    {
        return LoopFloatConstrain(Ang, -180.0f, 180.0f);
    }

    int FloatRounding(float raw)
    {
        static int integer;
        static float decimal;
        integer = (int)raw;
        decimal = raw - integer;
        if (decimal > 0.5f)
            integer++;
        return integer;
    }

    int float_to_uint(float x_float, float x_min, float x_max, int bits)
    {
        /* Converts a float to an unsigned int, given range and number of bits */
        float span = x_max - x_min;
        float offset = x_min;
        return (int)((x_float - offset) * ((float)((1 << bits) - 1)) / span);
    }

    float uint_to_float(int x_int, float x_min, float x_max, int bits)
    {
        /* converts unsigned int to float, given range and number of bits */
        float span = x_max - x_min;
        float offset = x_min;
        return ((float)x_int) * span / ((float)((1 << bits) - 1)) + offset;
    }
}

namespace Matrix
{
    void MatInit(mat *m, uint8_t row, uint8_t col)
    {
        m->numCols = col;
        m->numRows = row;
        m->pData = (float *)malloc(row * col * sizeof(float));
    }

    // 三维向量归一化
    float *Norm3d(float *v)
    {
        float len = Numeric::Sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
        return v;
    }

    // 计算模长
    float NormOf3d(float *v)
    {
        return Numeric::Sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    }

    // 三维向量叉乘v1 x v2
    void Cross3d(float *v1, float *v2, float *res)
    {
        res[0] = v1[1] * v2[2] - v1[2] * v2[1];
        res[1] = v1[2] * v2[0] - v1[0] * v2[2];
        res[2] = v1[0] * v2[1] - v1[1] * v2[0];
    }

    // 三维向量点乘
    float Dot3d(float *v1, float *v2)
    {
        return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
    }
}