#ifndef XROBOT_IMU_HPP
#define XROBOT_IMU_HPP

#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

/* Structure for 3D vector (e.g., acceleration, gyroscope) */
typedef struct __attribute__((packed)) {
    float x;
    float y;
    float z;
} Vector3;
  
/* Structure for quaternion representation of rotation */
typedef struct __attribute__((packed)) {
    float q0;
    float q1;
    float q2;
    float q3;
} Quaternion;

/* Structure for Euler angles representation of rotation */
typedef struct __attribute__((packed)) {
    float yaw;
    float pit;
    float rol;
} EulerAngles;

/* Full data structure received from IMU sensor */
typedef struct __attribute__((packed)) {
    uint8_t prefix; /* Packet header (0xA5) */
    uint8_t id;     /* IMU device ID (0x30) */
    uint32_t time;  /* Timestamp */
    Quaternion quat_;
    Vector3 gyro_;
    Vector3 accl_;
    EulerAngles eulr_;
    uint8_t crc8; /* CRC-8 checksum */
} Data;

/* CAN Data Structure */
typedef struct __attribute__((packed)) {
    uint8_t prefix;
    uint32_t id;
    uint8_t data[8];
    uint8_t crc8;
} DataCanToUart;

union xrobot_data_union
{
    uint8_t RxBuffer[sizeof(Data)];
    Data data_struct;
} __attribute__((packed));


class XROBOT_IMU
{
public:
    XROBOT_IMU();
    ~XROBOT_IMU();

    xrobot_data_union xrobot_data;

    void ProcessPacket(uint8_t* data, uint16_t len);

    /*------------------------------------------------实例函数------------------------------------------------*/
    /**
     * @brief XROBOT_IMU实例
     * @return XROBOT_IMU实例指针
     */
    static XROBOT_IMU *Instance()
    {
        static XROBOT_IMU instance;
        return &instance;
    }
};

#endif // XROBOT_IMU_HPP
