//
// Created by cosmosmount on 2025/9/2.
//

#include "BMI088.hpp"
#include "bsp_spi.hpp"

#include "ServiceIMU.hpp"
#include "bsp_pwm.hpp"
#include "bsp_dwt.hpp"


namespace BMI088
{
    /**
     * @brief BMI088 acc gyro 标定
     * @note 标定后的数据存储在bmi088->bias和gNorm中,用于后续数据消噪和单位转换归一化
     * @attention 不管工作模式是blocking还是IT,标定时都是blocking模式,所以不用担心中断关闭后无法标定(RobotInit关闭了全局中断)
     * @attention 标定精度和等待时间有关,目前使用线性回归.后续考虑引入非线性回归
     * @todo 将标定次数(等待时间)变为参数供设定
     * @section 整体流程为1.累加加速度数据计算gNrom()
     *                   2.累加陀螺仪数据计算零飘
     *                   3. 如果标定过程运动幅度过大,重新标定
     *                   4.保存标定参数
     */
    void cBMI088::Calibrate()
    {
        Acc_coef = IMU_ACCEL_3G_SEN; // 标定完后要乘以9.805/gNorm，注意这里需要和配置的范围对应

        // 一次性参数用完就丢,不用static
        float startTime;                     // 开始标定时间,用于确定是否超时
        uint16_t CaliTimes = 20;           // 标定次数(6s)
        float gyroMax[3], gyroMin[3];        // 保存标定过程中读取到的数据最大值判断是否满足标定环境
        float gNormTemp, gNormMax, gNormMin; // 同上,计算矢量范数(模长)
        float gyroDiff[3], gNormDiff;        // 每个轴的最大角速度跨度及其模长


        startTime = DWT_GetTimeline_s();
        // 循环继续的条件为标定环境不满足
        do // 用do while至少执行一次,省得对上面的参数进行初始化
        {  // 标定超时,直接使用预标定参数(如果有)
            if (DWT_GetTimeline_s() - startTime > 0.01)
            { // 两次都没有成功就切换标定模式,丢给下一个if处理,使用预标定参数
                self_test.CALIBRATE_ERR = true;
                break;
            }

            DWT_Delay(0.0005);
            gNorm = 0;
            for (uint8_t i = 0; i < 3; i++) // 重置gNorm和零飘
                Gyro_offset[i] = 0;

            // @todo : 这里也有获取bmi088数据的操作,后续与BMI088Acquire合并.注意标定时的工作模式是阻塞,且offset和acc_coef要初始化成0和1,标定完成后再设定为标定值
            for (uint16_t i = 0; i < CaliTimes; ++i) // 提前计算,优化
            {
                ReadAccData(&acc_data);
                ReadGyroData(&gyro_data);

				gNormTemp = Numeric::Sqrt(acc_data.x * acc_data.x +
                         acc_data.y * acc_data.y +
                         acc_data.z * acc_data.z); // 计算加速度范数

                gNorm += gNormTemp; // 计算范数并累加,最后除以calib times获取单次值

                Gyro_offset[0] += gyro_data.x; // 因为标定时传感器静止,所以采集到的值就是漂移,累加当前值,最后除以calib times获得零飘
                Gyro_offset[1] += gyro_data.y;
                Gyro_offset[2] += gyro_data.z;

                if (i == 0) // 避免未定义的行为(else中)
                {
                    // 初始化成当前的重力加速度模长
                    gNormMax = gNormMin = gNormTemp;

                    // 初始化成当前的陀螺仪数据
                    gyroMax[0] = gyro_data.x;
                    gyroMax[1] = gyro_data.y;
                    gyroMax[2] = gyro_data.z;

                    gyroMin[0] = gyro_data.x;
                    gyroMin[1] = gyro_data.y;
                    gyroMin[2] = gyro_data.z;
                }
                else // 更新gNorm的Min Max和gyro的minmax
                {
                    gNormMax = gNormMax > gNormTemp ? gNormMax : gNormTemp;
                    gNormMin = gNormMin < gNormTemp ? gNormMin : gNormTemp;

                    gyroMax[0] = gyroMax[0] > gyro_data.x ? gyroMax[0] : gyro_data.x;
                    gyroMin[0] = gyroMin[0] < gyro_data.x ? gyroMin[0] : gyro_data.x;

                    gyroMax[1] = gyroMax[1] > gyro_data.y ? gyroMax[1] : gyro_data.y;
                    gyroMin[1] = gyroMin[1] < gyro_data.y ? gyroMin[1] : gyro_data.y;

                    gyroMax[2] = gyroMax[2] > gyro_data.z ? gyroMax[2] : gyro_data.z;
                    gyroMin[2] = gyroMin[2] < gyro_data.z ? gyroMin[2] : gyro_data.z;
                }

                gNormDiff = gNormMax - gNormMin; // 最大值和最小值的差
                for (uint8_t j = 0; j < 3; ++j)
                    gyroDiff[j] = gyroMax[j] - gyroMin[j]; // 分别计算三轴
                if (gNormDiff > 0.5f ||
                    gyroDiff[0] > 0.15f ||
                    gyroDiff[1] > 0.15f ||
                    gyroDiff[2] > 0.15f)
                    break;         // 超出范围了,重开! remake到while循环,外面还有一层
                DWT_Delay(0.0005); // 休息一会再开始下一轮数据获取,IMU准备数据需要时间
            }
            gNorm /= (float)CaliTimes; // 加速度范数重力
            for (uint8_t i = 0; i < 3; ++i)
                Gyro_offset[i] /= (float)CaliTimes; // 三轴零飘
            // 这里直接存到temperature,可以另外增加BMI088Instance的成员变量TempWhenCalib
            // temperature = raw_data.temperature * BMI088_TEMP_FACTOR + BMI088_TEMP_OFFSET; // 保存标定时的温度,如果已知温度和零飘的关系
        } while (gNormDiff > 0.5f ||
                 fabsf(gNorm - 9.8f) > 0.5f ||
                 gyroDiff[0] > 0.15f ||
                 gyroDiff[1] > 0.15f ||
                 gyroDiff[2] > 0.15f ||
                 fabsf(Gyro_offset[0]) > 0.01f ||
                 fabsf(Gyro_offset[1]) > 0.01f ||
                 fabsf(Gyro_offset[2]) > 0.01f); // 满足条件说明标定环境不好

        if (self_test.CALIBRATE_ERR == true) // 如果标定失败，使用预标定参数
        {
            Gyro_offset[0] = BMI088_PRE_CALI_ACC_X_OFFSET;
            Gyro_offset[1] = BMI088_PRE_CALI_ACC_Y_OFFSET;
            Gyro_offset[2] = BMI088_PRE_CALI_ACC_Z_OFFSET;
            gNorm = BMI088_PRE_CALI_G_NORM;
        }
        Acc_coef *= 9.805 / gNorm;
    }

    void cBMI088::TemperatureControl(float target_temp)
    {
        TempPid.ref = target_temp;
        TempPid.fdb = TempFdbFilter.Update(acc_data.temperature);
        TempPid.UpdateResult();

        if (TempPid.result < 0)
        {
            PWM_SetDutyRatio(&HEATING_RESISTANCE_TIM, 0, TIM_CHANNEL_1);
        }
        else
        {
            PWM_SetDutyRatio(&HEATING_RESISTANCE_TIM, TempPid.result / 999, TIM_CHANNEL_1);
        }
    }

    void cBMI088::VerifyAccChipID()
    {
        uint8_t pRxData[2]; //< 读取两个字节,第一个字节是dummy data,第二个字节是chip id

        ReadReg(BMI088_CS_ACC, ACC_CHIP_ID_ADDR, pRxData, 2); //< 读取加速度计chip id
        tx_thread_sleep(1);
        //< 如果chip id不等于预设值,则加速度计ID错误,初始化错误
        if (pRxData[1] != ACC_CHIP_ID_VAL)
        {
            self_test.ACC_CHIP_ID_ERR = true;
            self_test.INIT_ERR = true;
        }
        else if (pRxData[1] == ACC_CHIP_ID_VAL)
        {
            self_test.ACC_CHIP_ID_ERR = false;
        }
    }

    void cBMI088::VerifyGyroChipID()
    {
        uint8_t pRxData;                                                 //< 读取一个字节,chip id
        ReadReg(BMI088_CS_GYRO, GYRO_CHIP_ID_ADDR, &pRxData, 1); //< 读取陀螺仪chip id
        tx_thread_sleep(1);
        //< 如果chip id不等于预设值,则陀螺仪ID错误,初始化错误
        if (pRxData != GYRO_CHIP_ID_VAL)
        {
            self_test.GYRO_CHIP_ID_ERR = true;
            self_test.INIT_ERR = true;
        }
        else if (pRxData == GYRO_CHIP_ID_VAL)
        {
            self_test.GYRO_CHIP_ID_ERR = false;
        }
    }

    void cBMI088::VerifyAccData() {}

    void cBMI088::VerifyGyroData() {}

    void cBMI088::WriteReg(enum BMI088_SENSOR cs, uint8_t addr, uint8_t *data, uint8_t len)
    {
        if (cs == BMI088_CS_ACC)
            HAL_GPIO_WritePin(BMI088_ACC_GPIOx, BMI088_ACC_GPIOp, GPIO_PIN_RESET);
        else if (cs == BMI088_CS_GYRO)
            HAL_GPIO_WritePin(BMI088_GYRO_GPIOx, BMI088_GYRO_GPIOp, GPIO_PIN_RESET);

        uint8_t pTxData = (addr & BMI088_SPI_WRITE_CODE); //< 处理地址为写地址

        HAL_SPI_Transmit(&BMI088_SPI, &pTxData, 1, 1000); //< 发送地址
        HAL_SPI_Transmit(&BMI088_SPI, data, len, 1000);   //< 发送数据

        // 理论上，这里不需要延时，但是如果数据出现问题，请尝试增加延时
        if (self_test.INIT_ERR == true)
        {
            DWT_Delay(0.001);
        }
        //< 取消片选
        if (cs == BMI088_CS_ACC)
            HAL_GPIO_WritePin(BMI088_ACC_GPIOx, BMI088_ACC_GPIOp, GPIO_PIN_SET);
        else if (cs == BMI088_CS_GYRO)
            HAL_GPIO_WritePin(BMI088_GYRO_GPIOx, BMI088_GYRO_GPIOp, GPIO_PIN_SET);
    }

    void cBMI088::ReadReg(enum BMI088_SENSOR cs, uint8_t addr, uint8_t *data, uint8_t len)
    {
        if (cs == BMI088_CS_ACC)
            HAL_GPIO_WritePin(BMI088_ACC_GPIOx, BMI088_ACC_GPIOp, GPIO_PIN_RESET);
        else if (cs == BMI088_CS_GYRO)
            HAL_GPIO_WritePin(BMI088_GYRO_GPIOx, BMI088_GYRO_GPIOp, GPIO_PIN_RESET);

        uint8_t pTxData = (addr | BMI088_SPI_READ_CODE); //< 处理地址为读地址

        HAL_SPI_Transmit(&BMI088_SPI, &pTxData, 1, 1000); //< 发送地址
        HAL_SPI_Receive(&BMI088_SPI, data, len, 1000);    //< 读取数据

        //< 取消片选
        if (cs == BMI088_CS_ACC)
            HAL_GPIO_WritePin(BMI088_ACC_GPIOx, BMI088_ACC_GPIOp, GPIO_PIN_SET);
        else if (cs == BMI088_CS_GYRO)
            HAL_GPIO_WritePin(BMI088_GYRO_GPIOx, BMI088_GYRO_GPIOp, GPIO_PIN_SET);
    }

    void cBMI088::Config()
    {
        tx_thread_sleep(10); //< 等待系统稳定
        //< 加速度计初始化
        //< 先软重启，清空所有寄存器
        uint8_t pTxData;
        pTxData = ACC_SOFTRESET_VAL;
        WriteReg(BMI088_CS_ACC, ACC_SOFTRESET_ADDR, &pTxData, 1);
        tx_thread_sleep(150); //< 延时100ms,重启需要时间

        //< 打开加速度计电源
        pTxData = ACC_PWR_CTRL_ON;
        WriteReg(BMI088_CS_ACC, ACC_PWR_CTRL_ADDR, &pTxData, 1);
        tx_thread_sleep(150); //

        //< 加速度计变成正常模式
        pTxData = ACC_PWR_CONF_ACT;
        WriteReg(BMI088_CS_ACC, ACC_PWR_CONF_ADDR, &pTxData, 1);
        tx_thread_sleep(10); //

        //< 测量范围
        pTxData = ACC_RANGE_3G;
        WriteReg(BMI088_CS_ACC, ACC_RANGE_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0xAC;
        WriteReg(BMI088_CS_ACC, ACC_CONF_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x08;
        WriteReg(BMI088_CS_ACC, INT1_IO_CTRL_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x04;
        WriteReg(BMI088_CS_ACC, INT_MAP_DATA_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        /*-------------------------------------陀螺仪初始化-------------------------------------*/
        //< 先软重启，清空所有寄存器
        pTxData = GYRO_SOFTRESET_VAL;
        WriteReg(BMI088_CS_GYRO, GYRO_SOFTRESET_ADDR, &pTxData, 1);
        tx_thread_sleep(100); //< 延时100ms,重启需要时间

        pTxData = GYRO_RANGE_1000_DEG_S;
        WriteReg(BMI088_CS_GYRO, GYRO_RANGE_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x02;//GYRO_ODR_1000Hz_BANDWIDTH_116Hz | GYRO_LPM1_SUS;
        WriteReg(BMI088_CS_GYRO, GYRO_BANDWIDTH_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = GYRO_LPM1_NOR;
        WriteReg(BMI088_CS_GYRO, GYRO_LPM1_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x80;
        WriteReg(BMI088_CS_GYRO, GYRO_INT_CTRL_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x0C;
        WriteReg(BMI088_CS_GYRO, GYRO_INT3_INT4_IO_CONF_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms

        pTxData = 0x01;
        WriteReg(BMI088_CS_GYRO, GYRO_INT3_INT4_IO_MAP_ADDR, &pTxData, 1);
        tx_thread_sleep(5); //< 延时5ms
    }


    void cBMI088::ReadAccData(acc_data_t *data)
    {
        uint8_t buf[ACC_XYZ_LEN + 1];                                         //< 读取数据缓存
        int16_t acc[3];                                                       //< 加速度计数据暂存
        ReadReg(BMI088_CS_ACC, ACC_X_LSB_ADDR, buf, ACC_XYZ_LEN + 1); //< 读取加速度计数据
        //< 拼接和转换数据
        acc[0] = ((int16_t)buf[1 + 1] << 8) + (int16_t)buf[0 + 1];
        acc[1] = ((int16_t)buf[3 + 1] << 8) + (int16_t)buf[2 + 1];
        acc[2] = ((int16_t)buf[5 + 1] << 8) + (int16_t)buf[4 + 1];
        data->x = sensor_filter[0].Update((float)acc[0] * Acc_coef);
        data->y = sensor_filter[1].Update((float)acc[1] * Acc_coef);
        data->z = sensor_filter[2].Update((float)acc[2] * Acc_coef);
        data->x = (float)acc[0] * Acc_coef;
        data->y = (float)acc[1] * Acc_coef;
        data->z = (float)acc[2] * Acc_coef;
    }

    void cBMI088::ReadGyroData(gyro_data_t *data)
    {
        uint8_t buf[GYRO_XYZ_LEN]; //, range; //< 读取数据缓存, range其实是写入的配置
        int16_t gyro[3];

        ReadReg(BMI088_CS_GYRO, GYRO_RATE_X_LSB_ADDR, buf, GYRO_XYZ_LEN);
        //< 拼接和转换数据
        gyro[0] = ((int16_t)buf[1] << 8) + (int16_t)buf[0];
        gyro[1] = ((int16_t)buf[3] << 8) + (int16_t)buf[2];
        gyro[2] = ((int16_t)buf[5] << 8) + (int16_t)buf[4];
        // 注意这里就不要又除法又乘法的了，直接乘以一个常数，根据配置，单位是16.384，所以直接乘以1/16.384 * DEG2SEC即可
        data->x = sensor_filter[3].Update((float)gyro[0] * IMU_GYRO_1000_SEN);
        data->y = sensor_filter[4].Update((float)gyro[1] * IMU_GYRO_1000_SEN);
        data->z = sensor_filter[5].Update((float)gyro[2] * IMU_GYRO_1000_SEN);
        // data->x = (float)gyro[0] * IMU_GYRO_2000_SEN;
        // data->y = (float)gyro[1] * IMU_GYRO_2000_SEN;
        // data->z = (float)gyro[2] * IMU_GYRO_2000_SEN;
    }

    void cBMI088::ReadAccTemperature(float *temp)
    {
        uint8_t buf[TEMP_LEN + 1];
        ReadReg(BMI088_CS_ACC, TEMP_MSB_ADDR, buf, TEMP_LEN + 1);
        uint16_t temp_uint11 = (buf[0 + 1] << 3) + (buf[1 + 1] >> 5);
        int16_t temp_int11;
        if (temp_uint11 > 1023)
        {
            temp_int11 = (int16_t)temp_uint11 - 2048;
        }
        else
        {
            temp_int11 = (int16_t)temp_uint11;
        }
        *temp = temp_int11 * TEMP_UNIT + TEMP_BIAS;
    }

}

