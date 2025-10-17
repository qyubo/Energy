//
// Created by cosmosmount on 2025/9/2.
//

#ifndef BMI088_HPP
#define BMI088_HPP

#include "IMU.hpp"
#include "pid.hpp"
#include "filter.hpp"

using namespace Filter;

namespace BMI088
{
/**
* @struct bmi088_data_t
* @brief BMI088数据结构体
* 存储BMI088的数据
* @param acc_data 加速度计数据
* @param gyro_data 陀螺仪数据
*/
typedef struct bmi088_data_t
{
    acc_data_t acc_data;   // 加速度计数据
    gyro_data_t gyro_data; // 陀螺仪数据
} bmi088_data_t;

/**
 * @enum BMI088_SENSOR
 * @brief BMI088传感器的片选
 * @param BMI088_CS_ACC 加速度计片选
 * @param BMI088_CS_GYRO 陀螺仪片选
 */
enum BMI088_SENSOR
{
    BMI088_CS_ACC = 0,
    BMI088_CS_GYRO = 1,
};

/*---------------------------相关宏定义---------------------------*/
#define BMI088_SPI hspi1 //< SPI句柄

#define BMI088_ACC_GPIOx GPIOA      //< 加速度计片选端口
#define BMI088_ACC_GPIOp GPIO_PIN_4 //< 加速度计片选引脚

#define BMI088_GYRO_GPIOx GPIOB      //< 陀螺仪片选端口
#define BMI088_GYRO_GPIOp GPIO_PIN_0 //< 陀螺仪片选引脚

#define HEATING_RESISTANCE_TIM htim10            //< 加热电阻定时器
#define HEATING_RESISTANCE_CHANNEL TIM_CHANNEL_1 //< 加热电阻通道

/*-----bmi088的spi读取协议部分-----*/
#define BMI088_SPI_WRITE_CODE 0x7F
#define BMI088_SPI_READ_CODE 0x80

/*-----加速度计寄存器表-----*/
#define ACC_CHIP_ID_ADDR 0x00
#define ACC_CHIP_ID_VAL 0x1E

#define ACC_ERR_REG_ADDR 0x02

#define ACC_STATUS_ADDR 0x03

#define ACC_X_LSB_ADDR 0x12
#define ACC_X_MSB_ADDR 0x13
#define ACC_Y_LSB_ADDR 0x14
#define ACC_Y_MSB_ADDR 0x15
#define ACC_Z_LSB_ADDR 0x16
#define ACC_Z_MSB_ADDR 0x17
#define ACC_XYZ_LEN 6

#define SENSORTIME_0_ADDR 0x18
#define SENSORTIME_0_UNIT (39.0625f / 1000000.0f)
#define SENSORTIME_1_ADDR 0x19
#define SENSORTIME_1_UNIT (10.0 / 1000.0f)
#define SENSORTIME_2_ADDR 0x1A
#define SENSORTIME_2_UNIT (2.56f)
#define SENSORTIME_LEN 3

#define ACC_INT_STAT_1_ADDR 0x1D

#define TEMP_MSB_ADDR 0x22
#define TEMP_LSB_ADDR 0x23
#define TEMP_LEN 2
#define TEMP_UNIT 0.125f
#define TEMP_BIAS 23.0f

#define ACC_CONF_ADDR 0x40
#define ACC_CONF_RESERVED 0x01
#define ACC_CONF_BWP_OSR4 0x00
#define ACC_CONF_BWP_OSR2 0x01
#define ACC_CONF_BWP_NORM 0x02
#define ACC_CONF_ODR_12_5_Hz 0x05
#define ACC_CONF_ODR_25_Hz 0x06
#define ACC_CONF_ODR_50_Hz 0x07
#define ACC_CONF_ODR_100_Hz 0x08
#define ACC_CONF_ODR_200_Hz 0x09
#define ACC_CONF_ODR_400_Hz 0x0A
#define ACC_CONF_ODR_800_Hz 0x0B
#define ACC_CONF_ODR_1600_Hz 0x0C

#define ACC_RANGE_ADDR 0x41
#define ACC_RANGE_3G 0x00
#define ACC_RANGE_6G 0x01
#define ACC_RANGE_12G 0x02
#define ACC_RANGE_24G 0x03

#define INT1_IO_CTRL_ADDR 0x53

#define INT2_IO_CTRL_ADDR 0x54

#define INT_MAP_DATA_ADDR 0x58

#define ACC_SELF_TEST_ADDR 0x6D
#define ACC_SELF_TEST_OFF 0x00
#define ACC_SELF_TEST_POS 0x0D
#define ACC_SELF_TEST_NEG 0x09

#define ACC_PWR_CONF_ADDR 0x7C
#define ACC_PWR_CONF_SUS 0x03
#define ACC_PWR_CONF_ACT 0x00

#define ACC_PWR_CTRL_ADDR 0x7D
#define ACC_PWR_CTRL_ON 0x04
#define ACC_PWR_CTRL_OFF 0x00

#define ACC_SOFTRESET_ADDR 0x7E
#define ACC_SOFTRESET_VAL 0xB6

/*-----陀螺仪寄存器表-----*/
#define GYRO_CHIP_ID_ADDR 0x00
#define GYRO_CHIP_ID_VAL 0x0F

#define GYRO_RATE_X_LSB_ADDR 0x02
#define GYRO_RATE_X_MSB_ADDR 0x03
#define GYRO_RATE_Y_LSB_ADDR 0x04
#define GYRO_RATE_Y_MSB_ADDR 0x05
#define GYRO_RATE_Z_LSB_ADDR 0x06
#define GYRO_RATE_Z_MSB_ADDR 0x07
#define GYRO_XYZ_LEN 6

#define GYRO_INT_STAT_1_ADDR 0x0A

#define GYRO_RANGE_ADDR 0x0F
#define GYRO_RANGE_2000_DEG_S 0x00
#define GYRO_RANGE_1000_DEG_S 0x01
#define GYRO_RANGE_500_DEG_S 0x02
#define GYRO_RANGE_250_DEG_S 0x03
#define GYRO_RANGE_125_DEG_S 0x04

#define GYRO_BANDWIDTH_ADDR 0x10
#define GYRO_ODR_2000Hz_BANDWIDTH_532Hz 0x00
#define GYRO_ODR_2000Hz_BANDWIDTH_230Hz 0x01
#define GYRO_ODR_1000Hz_BANDWIDTH_116Hz 0x02
#define GYRO_ODR_400Hz_BANDWIDTH_47Hz 0x03
#define GYRO_ODR_200Hz_BANDWIDTH_23Hz 0x04
#define GYRO_ODR_100Hz_BANDWIDTH_12Hz 0x05
#define GYRO_ODR_200Hz_BANDWIDTH_64Hz 0x06
#define GYRO_ODR_100Hz_BANDWIDTH_32Hz 0x07

#define GYRO_LPM1_ADDR 0x11
#define GYRO_LPM1_NOR 0x00
#define GYRO_LPM1_SUS 0x80
#define GYRO_LPM1_DEEP_SUS 0x20

#define GYRO_SOFTRESET_ADDR 0x14
#define GYRO_SOFTRESET_VAL 0xB6

#define GYRO_INT_CTRL_ADDR 0x15

#define GYRO_INT3_INT4_IO_CONF_ADDR 0x16

#define GYRO_INT3_INT4_IO_MAP_ADDR 0x18

#define GYRO_SELF_TEST_ADDR 0x3C
#define GYRO_SELF_TEST_ON 0x01

/* pre calibrate parameter to go here */
#define BMI088_PRE_CALI_ACC_X_OFFSET 0.007510f
#define BMI088_PRE_CALI_ACC_Y_OFFSET 0.000985f
#define BMI088_PRE_CALI_ACC_Z_OFFSET 0.010313f
#define BMI088_PRE_CALI_G_NORM 9.805f

class cBMI088: public cIMU
{
    public:
        // imu_error_t bmi088_selfTest; // BMI088错误结构体
        // bmi088_data_t bmi088_data;   // BMI088数据结构体
        /**
         * @brief 从寄存器读取数据
         * @param cs 片选
         * @param addr 寄存器地址
         * @param data 数据
         * @param len 数据长度
         */
        void ReadReg(enum BMI088_SENSOR cs, uint8_t addr, uint8_t *data, uint8_t len);

        /**
         * @brief 写数据到寄存器
         * @param cs 片选
         * @param addr 寄存器地址
         * @param data 数据
         * @param len 数据长度
         */
        void WriteReg(enum BMI088_SENSOR cs, uint8_t addr, uint8_t *data, uint8_t len);

        void Config() override;
        void Calibrate() override;

        void ReadAccData(acc_data_t *data) override;
        void ReadAccTemperature(float *temp) override;
        void ReadGyroData(gyro_data_t *data) override;

        void VerifyAccChipID() override;
        void VerifyGyroChipID() override;
        void VerifyAccData() override;
        void VerifyGyroData() override;

        void TemperatureControl(float target_temp) override;



    private:
        float Gyro_offset[3]; // 陀螺仪零飘
        float Acc_coef = IMU_ACCEL_3G_SEN;       // 加速度计灵敏度，标定完后要乘以9.805/gNorm
        float gNorm = 9.805f;          // 重力加速度模长
        IIRFilter sensor_filter[6] = {
            IIRFilter(2,LOWPASS,333),
            IIRFilter(2,LOWPASS,333),
            IIRFilter(2,LOWPASS,333),
            IIRFilter(2,LOWPASS,333),
            IIRFilter(2,LOWPASS,333),
            IIRFilter(2,LOWPASS,333)
        };
    };

}

#endif //BMI088_HPP
