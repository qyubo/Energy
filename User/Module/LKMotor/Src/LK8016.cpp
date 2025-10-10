#include "LK8016.hpp"
#include "bsp_can.hpp"
// #include "FirstOrderFilter.hpp"
/**
 * @brief GM6020类的构造函数。
 * 初始化电机的控制模式、各种设定值和PID控制器。
 */
//  FirstOrderFilter lk9025spdfilter;
 
LK8016::LK8016() : LKMotor(LK8016_TYPE)
{
    // 初始化为松开模式
    controlMode = RELAX_MODE;

    // 初始化设定值为0
    speedSet = 0;
    positionSet = 0;
    currentSet = 0;
    maxCurrent = 2000; // 电机最大电流设定

    // 初始化电机反馈数据
    motorFeedback.speedFdb = 0;
    motorFeedback.lastSpeedFdb = 0;
    motorFeedback.positionFdb = 0;
    motorFeedback.lastPositionFdb = 0;
    motorFeedback.temperatureFdb = 0;

    // PID控制器初始化
    speedPid.mode = PID_POSITION;
    speedPid.kp = 0.1;
    speedPid.ki = 0.0;
    speedPid.kd = 0.0;
    speedPid.maxOut = 25000;
    speedPid.maxIOut = 3;

    positionPid.mode = PID_POSITION;
    positionPid.kp = 0.1;
    positionPid.ki = 0.0;
    positionPid.kd = 0.0;
    positionPid.maxOut = 25000;
    positionPid.maxIOut = 3;
		
		// lk9025spdfilter.SetTau(0.1f);
		// lk9025spdfilter.SetUpdatePeriod(1.0f);
}

/**
 * @brief GM6020类的析构函数。
 */
LK8016::~LK8016()
{
}

/**
 * @brief 设置电机输出。
 * 根据当前控制模式，计算并设置电机的当前输出。
 */
void LK8016::setOutput()
{

		
    if (this->controlMode == RELAX_MODE)
    {
        this->currentSet = 0.0; // 松开模式下电流设定为0
				return;
    }
    else if (this->controlMode == TOR_MODE)
    {
        // currentSet = (torqueSet * 2000) / (0.24f * 32.0f * 6.0f); 0.24：扭矩常数，6：减速比，32.0：电流范围，2000.0：电流数值范围
        this->currentSet = this->torqueSet * 43.4028f;
				this->currentSet = Numeric::FloatConstrain(currentSet, -2000, 2000);
				return;
    } 
    else if (this->controlMode == POS_MODE)
    {
       // 外环控制，位置环控制
       this->positionPid.ref = this->positionSet;
       this->positionPid.fdb = this->motorFeedback.positionFdb;
				// 检查并调整positionSet值在有效范围内
				if (this->positionSet < -Numeric::Pi)
				{
						this->positionSet += 2 * Numeric::Pi; // 调整使其在-π到π范围内
				}
				else if (this->positionSet > Numeric::Pi)
				{
						this->positionSet -= 2 * Numeric::Pi; // 调整使其在-π到π范围内
				}
       this->positionPid.UpdateResult();

        // 内环控制，速度环控制
				this->speedPid.ref = this->positionPid.result;
				this->speedPid.fdb = this->motorFeedback.speedFdb;
        this->speedPid.UpdateResult();

        this->currentSet = this->speedPid.result; // 根据速度PID结果设置电流
    }

    else
    {
        this->currentSet = 0.0; // 其他情况电流设定为0
				return;
    }

    // 限制电流输出不超过最大值
    this->currentSet = Numeric::FloatConstrain(this->currentSet, -maxCurrent, maxCurrent);
}

void LK8016::UpdateSensorData(uint8_t *buffer_ptr)
{
    motorFeedback.last_ecd = motorFeedback.ecd;     ///< 上一次的编码器值
    motorFeedback.ecd = (uint16_t)(buffer_ptr[7] << 8 | buffer_ptr[6]);          ///< 编码器值
    motorFeedback.speed_dps = (int16_t)(buffer_ptr[5] << 8 | buffer_ptr[4]);     ///< 速度值，单位dps, degree per second
    motorFeedback.currentFdb = (int16_t)(buffer_ptr[3] << 8 | buffer_ptr[2]); ///< 电流值，或者说是转矩值
    motorFeedback.temperatureFdb = buffer_ptr[1];                                  ///< 温度值

    /* update last time speed and position -------------------------------------------*/
    motorFeedback.lastPositionFdb = motorFeedback.positionFdb;
    motorFeedback.lastSpeedFdb = motorFeedback.speedFdb;

    motorFeedback.positionFdb = Numeric::LoopFloatConstrain((float)(motorFeedback.ecd - offset) * LKMotor::RawPos2Rad, -Numeric::Pi, Numeric::Pi);
    motorFeedback.speedFdb = motorFeedback.speed_dps * LKMotor::RawDps2Rpsps;

    motorFeedback.torqueFdb = motorFeedback.currentFdb * 0.0230399882f;
}




