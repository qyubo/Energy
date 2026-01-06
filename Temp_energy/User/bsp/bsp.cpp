#include "bsp.hpp"
#include "stm32f1xx_hal_can.h"
void bsp_init()
{
  CAN_Init();    // CAN初始化
  DWT_Init(72); // 168MHz，c板主频168MHz
}
