#ifndef AHRS_HPP
#define AHRS_HPP


#include "arm_math.h"

namespace AHRS
{
    static constexpr int X = 0;
    static constexpr int Y = 1;
    static constexpr int Z = 2;

    /**
     * @brief 四元数更新函数,即实现dq/dt=0.5Ωq
     *
     * @param q  四元数
     * @param gx
     * @param gy
     * @param gz
     * @param dt 距离上次调用的时间间隔
     */
    void QuaternionUpdate(float *q, float gx, float gy, float gz, float dt);

    /**
     * @brief ZYX欧拉角转换为四元数
     *
     * @param Yaw
     * @param Pitch
     * @param Roll
     * @param q
     */
    void EularAngleToQuaternion(float Yaw, float Pitch, float Roll, float *q);

    /**
     * @brief 机体系到惯性系的变换函数
     *在·
     * @param vecBF body frame
     * @param vecEF earth frame
     * @param q
     */
    void BodyFrameToEarthFrame(const float *vecBF, float *vecEF, float *q);

    /**
     * @brief 惯性系转换到机体系
     *
     * @param vecEF
     * @param vecBF
     * @param q
     */
    void EarthFrameToBodyFrame(const float *vecEF, float *vecBF, float *q);

};

#endif // AHRS_HPP
