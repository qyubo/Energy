#include "XRobot.hpp"
#include "crc.hpp"

XROBOT_IMU::XROBOT_IMU() {
     
}

XROBOT_IMU::~XROBOT_IMU() {}

#define DATA_IMU_LENGTH sizeof(Data)

void XROBOT_IMU::ProcessPacket(uint8_t* data, uint16_t len)
{
    uint8_t prefix = data[0];
    if(prefix == 0xA5)
    {
      if (Verify_CRC8_Check_Sum(data,sizeof(data)))
      {
        memcpy(&xrobot_data, data, sizeof(Data));
      }
    }
    
}

