#include "can_portocal_second.hpp"
#include "main.h"
#include "string.h"
#include "bsp_can.hpp"
//uint8_t can_Dafu_data[8];
void get_Centrol_measure(Board_Order_info_t *ptr,uint8_t can_rx_data[8])                                    
{                                                                   
	ptr->Energy_machine_state = can_rx_data[0];																
	ptr->All_Led_Close = can_rx_data[1];  
	ptr->All_Led_Open =  can_rx_data[2];  
	ptr->Board_Work =  can_rx_data[3];  
	ptr->Set_Color  = can_rx_data[4];
	ptr->start_permission = can_rx_data[5];
	ptr->RainBow_Flag =  can_rx_data[6];
}

/*烧哪块取消哪块注释，从板接收函数*/
void Board_RX(uint32_t ID, Board_Order_info_t data[5],uint8_t can_rx_data[8],Board_info_t Board_info)
{
	Board_info.offline_cnt_max = OFFLINE_TIME_MAX; 
	
	switch(ID)
	{
		
		case CAN_BOARD_ID_1_F:
		{
      CAN_Receive(can_rx_data,CAN_BOARD_ID_1_F);
			get_Centrol_measure(&data[0],can_rx_data);
			break;
		}	
	
	  case CAN_BOARD_ID_2_F:
		{
      CAN_Receive(can_rx_data,CAN_BOARD_ID_2_F);
			get_Centrol_measure(&data[1],can_rx_data);
			break;
		}	
		
		case CAN_BOARD_ID_3_F:
		{
      CAN_Receive(can_rx_data,CAN_BOARD_ID_3_F);
			get_Centrol_measure(&data[2],can_rx_data);

			
			break;
		}	
		
		case CAN_BOARD_ID_4_F:
		{
      CAN_Receive(can_rx_data,CAN_BOARD_ID_4_F);
			get_Centrol_measure(&data[3],can_rx_data);

			
			break;
		}	
		
		case CAN_BOARD_ID_5_F:
		{
 		 CAN_Receive(can_rx_data,CAN_BOARD_ID_5_F);
			get_Centrol_measure(&data[4],can_rx_data);
		
			break;
		}	
		
		default:
		{
				break;
		}
		
	}
}

/*从板发送函数*/
void Board_Tx(CAN_HandleTypeDef *hcan, uint32_t Id, uint8_t *msg, uint16_t len) {
    CAN_Transmit(hcan, Id, msg, len);
}
/*烧哪块取消哪块注释*/
void Board_Info_Tx(uint8_t can_Dafu_data[8], Big_Fu_info_t *Big_Fu_info) 
{
//    memcpy(can_Dafu_data, &Big_Fu_info[0], 8);
//    Board_Tx(&hcan, CAN_BOARD_ID_1, can_Dafu_data, 8);

    // 取消注释以发送到其他板
     memcpy(can_Dafu_data, &Big_Fu_info[1], 8);
     Board_Tx(&hcan, CAN_BOARD_ID_2, can_Dafu_data, 8);

//     memcpy(can_Dafu_data, &Big_Fu_info[2], 8);
//     Board_Tx(&hcan, CAN_BOARD_ID_3, can_Dafu_data, 8);

//     memcpy(can_Dafu_data, &Big_Fu_info[3], 8);
//     Board_Tx(&hcan, CAN_BOARD_ID_4, can_Dafu_data, 8);

//     memcpy(can_Dafu_data, &Big_Fu_info[4], 8);
//     Board_Tx(&hcan, CAN_BOARD_ID_5, can_Dafu_data, 8);
}


