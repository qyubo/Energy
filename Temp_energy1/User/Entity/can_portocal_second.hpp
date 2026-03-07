#ifndef CAN_PORTOCAL_SECOND_H
#define CAN_PORTOCAL_SECOND_H
#include "main.h"
#include "bsp_can.hpp"
#include<string.h>
#define OFFLINE_TIME_MAX   25
extern CAN_HandleTypeDef hcan;
typedef enum
{
    CAN_BOARD_ID_1 = 0x101,
    CAN_BOARD_ID_2 = 0x102,
    CAN_BOARD_ID_3 = 0x103,
    CAN_BOARD_ID_4 = 0x104,
	CAN_BOARD_ID_5 = 0x105,
	
	  CAN_BOARD_ID_1_F = 0x301,
    CAN_BOARD_ID_2_F = 0x302,
    CAN_BOARD_ID_3_F = 0x303,
    CAN_BOARD_ID_4_F = 0x304,
	  CAN_BOARD_ID_5_F = 0x305,

} can_msg_id_e;
/*五个扇叶待击打计数*/
typedef struct 
{
	uint8_t     B1_offline_cnt;
	uint8_t     B2_offline_cnt;
	uint8_t     B3_offline_cnt;
	uint8_t     B4_offline_cnt;
	uint8_t     B5_offline_cnt;
	uint8_t     offline_cnt_max;
	uint8_t     status;
}Board_info_t;
/*给从板发送的数据*/
typedef struct 
{
	uint8_t    Energy_machine_state;
	uint8_t    All_Led_Close;
	uint8_t    All_Led_Open;
	uint8_t    Board_Work;
	uint8_t    Set_Color;
	uint8_t    start_permission;
	uint8_t    RainBow_Flag;
	
}Board_Order_info_t;
/*给主板发送的数据*/
typedef struct
{   
	  uint8_t  	Color;                 //大符颜色
	  uint8_t  	Last_Color;            //上次状态下大符颜色
	  uint8_t   Hit_LED_state;         //LED击中状态，1-10对应环
	  uint8_t   Single_Hit_State;      //单个扇叶是否击中状态，1为击中0未击中
	  uint8_t   Single_Working_State;  //单个扇叶待击打状态 
	  uint8_t   All_Hit_State;         //全部扇叶击中状态
	  uint8_t   Spin_State;           //大符旋转状态
	  uint8_t   hit_time;
}Big_Fu_info_t;
extern Board_Order_info_t Board_Order_info[5];
extern Big_Fu_info_t Big_Fu_info[5];
//extern Board_info_t Board_info;
extern uint8_t can_rx_data[8];

#ifdef __cplusplus
extern "C" {
#endif

void get_Centrol_measure(Board_Order_info_t *ptr,uint8_t can_rx_data[8]);    
void Board_RX(uint32_t ID, Board_Order_info_t data[5],uint8_t can_rx_data[8],Board_info_t Board_info);
void Board_Tx(CAN_HandleTypeDef *hcan, uint32_t Id, uint8_t *msg, uint16_t len);
void Board_Info_Tx(uint8_t can_Dafu_data[8], Big_Fu_info_t *Big_Fu_info);
void CAN_Receive_server(uint8_t Rx_data[8],uint32_t Id);

#ifdef __cplusplus
}
#endif
#endif

