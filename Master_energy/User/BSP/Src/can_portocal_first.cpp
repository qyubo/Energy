#include "can_portocal_first.hpp"
#include "main.h"
#include "bsp_can.hpp"
#include "tx_api.h"

//#include "string.h"
uint8_t Can_Board_Order_data[8];
extern Board_Order_info_t Board_Order_info[5];

/*主板接收数据*/
void get_DAFU_measure(Big_Fu_info_t *ptr, uint8_t can_rx_data[8])                                    
{                                                                   
	ptr->Color = can_rx_data[0];																
	ptr->Last_Color = can_rx_data[1];  
	ptr->Hit_LED_state =  can_rx_data[2];  
	ptr->Single_Hit_State =  can_rx_data[3];                
	ptr->Single_Working_State = can_rx_data[4];      
 	ptr->All_Hit_State = can_rx_data[5];    
	ptr->Spin_State = can_rx_data[6];    
	ptr->hit_time= can_rx_data[7]; 
}
void Board_Info_Tx(void)
{
    // 直接循环 5 次，一口气发完
    for(int k = 0; k < 5; k++)
    {
        memcpy(Can_Board_Order_data, &Board_Order_info[k], 8);
        
        switch(k)
        {
            case 0: // 数组[0] -> 发给 1号板
                CAN_Transmit(&hcan1, CAN_BOARD_ID_1_F, Can_Board_Order_data, 0x08);
                break;
            case 1: // 数组[1] -> 发给 2号板
                CAN_Transmit(&hcan1, CAN_BOARD_ID_2_F, Can_Board_Order_data, 0x08);
                break;
            case 2: // 数组[2] -> 发给 3号板
                CAN_Transmit(&hcan1, CAN_BOARD_ID_3_F, Can_Board_Order_data, 0x08);
                break;
            case 3: // 数组[3] -> 发给 4号板
                CAN_Transmit(&hcan1, CAN_BOARD_ID_4_F, Can_Board_Order_data, 0x08);
                break;
            case 4: // 数组[4] -> 发给 5号板
                CAN_Transmit(&hcan1, CAN_BOARD_ID_5_F, Can_Board_Order_data, 0x08);
                break;
        }
        
        // tx_thread_sleep(1);
    }
}
