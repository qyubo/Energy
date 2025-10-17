#include "AHRS.hpp"

namespace AHRS {

    void QuaternionUpdate(float *q, float gx, float gy, float gz, float dt)
    {
        float qa, qb, qc;

        gx *= 0.5f * dt;
        gy *= 0.5f * dt;
        gz *= 0.5f * dt;
        qa = q[0];
        qb = q[1];
        qc = q[2];
        q[0] += (-qb * gx - qc * gy - q[3] * gz);
        q[1] += (qa * gx + qc * gz - q[3] * gy);
        q[2] += (qa * gy - qb * gz + q[3] * gx);
        q[3] += (qa * gz + qb * gy - qc * gx);
    }

    void EularAngleToQuaternion(float Yaw, float Pitch, float Roll, float *q)
    {
        Yaw /= 57.295779513f;
        Pitch /= 57.295779513f;
        Roll /= 57.295779513f;
        const float cosPitch = arm_cos_f32(Pitch / 2);
        const float cosYaw = arm_cos_f32(Yaw / 2);
        const float cosRoll = arm_cos_f32(Roll / 2);
        const float sinPitch = arm_sin_f32(Pitch / 2);
        const float sinYaw = arm_sin_f32(Yaw / 2);
        const float sinRoll = arm_sin_f32(Roll / 2);

        q[0] = sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw;
        q[1] = cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw;
        q[2] = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;
        q[3] = cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw;
    }

    void EarthFrameToBodyFrame(const float *vecEF, float *vecBF, float *q)
    {
        vecBF[0] = 2.0f * ((0.5f - q[2] * q[2] - q[3] * q[3]) * vecEF[0] +
                           (q[1] * q[2] + q[0] * q[3]) * vecEF[1] +
                           (q[1] * q[3] - q[0] * q[2]) * vecEF[2]);

        vecBF[1] = 2.0f * ((q[1] * q[2] - q[0] * q[3]) * vecEF[0] +
                           (0.5f - q[1] * q[1] - q[3] * q[3]) * vecEF[1] +
                           (q[2] * q[3] + q[0] * q[1]) * vecEF[2]);

        vecBF[2] = 2.0f * ((q[1] * q[3] + q[0] * q[2]) * vecEF[0] +
                           (q[2] * q[3] - q[0] * q[1]) * vecEF[1] +
                           (0.5f - q[1] * q[1] - q[2] * q[2]) * vecEF[2]);
    }

    void BodyFrameToEarthFrame(const float *vecBF, float *vecEF, float *q)
    {
        vecEF[0] = 2.0f * ((0.5f - q[2] * q[2] - q[3] * q[3]) * vecBF[0] +
                           (q[1] * q[2] - q[0] * q[3]) * vecBF[1] +
                           (q[1] * q[3] + q[0] * q[2]) * vecBF[2]);

        vecEF[1] = 2.0f * ((q[1] * q[2] + q[0] * q[3]) * vecBF[0] +
                           (0.5f - q[1] * q[1] - q[3] * q[3]) * vecBF[1] +
                           (q[2] * q[3] - q[0] * q[1]) * vecBF[2]);

        vecEF[2] = 2.0f * ((q[1] * q[3] - q[0] * q[2]) * vecBF[0] +
                           (q[2] * q[3] + q[0] * q[1]) * vecBF[1] +
                           (0.5f - q[1] * q[1] - q[2] * q[2]) * vecBF[2]);
    }
}