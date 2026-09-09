#ifndef CAN_PORTOCAL_FIRST_HPP
#define CAN_PORTOCAL_FIRST_HPP
#include "main.h"
#include "bsp_can.hpp"
#include<string.h>
/* Board_Work 指令定义（与从板 can_portocal_second.hpp 保持一致）：
   0=全灭，1~10=亮对应环，11=靶面+箭头流水，12=靶面全黑+流水灯半红 */
#define BOARD_DISPLAY_OFF              0U
#define BOARD_DISPLAY_TARGET_ARROW    11U
#define BOARD_DISPLAY_BLACK_HALF_RED  12U
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
typedef struct 
{
	uint8_t     offline_cnt_max;
	uint8_t     offline_cnt;
	uint8_t     status;
}motor_info_t;
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
#pragma pack(1)
typedef struct 
{
	uint8_t    Energy_machine_state;    // 大小能量机关判断，0关1小2大
	uint8_t    All_Led_Close;    // 全部LED关闭
	uint8_t    All_Led_Open;     // 全部LED打开
	uint8_t    Board_Work;       // 板工作状态0关，1-10为亮起的环，11为待击中十字
	uint8_t    Set_Color;        // 设置颜色
	uint8_t    start_permission; // 启动许可
	uint8_t    RainBow_Flag;     // 彩虹标志
	uint8_t 	reserved_padding;	  // 保留字节
} Board_Order_info_t;
#pragma pack()
/*给主板发送的数据*/
typedef struct
{   
	  uint8_t  	Color;                 //大符颜色
	  uint8_t  	Last_Color;            //上次状态下大符颜色
	  uint8_t   Hit_LED_state;         //LED击中或未击中状态
	  uint8_t   Single_Hit_State;      //单个扇叶是否击中状态，1为击中0未击中
	  uint8_t   Single_Working_State;  //单个扇叶待击打状态 
	  uint8_t   All_Hit_State;         //全部扇叶击中状态
	  uint8_t   Spin_State;           //大符旋转状态
	  uint8_t   hit_time;				//击打次数
}Big_Fu_info_t;
//extern Board_Order_info_t Board_Order_info[5];
//extern Big_Fu_info_t Big_Fu_info[5];
//extern Board_info_t Board_info;
// extern uint8_t can_rx_data[8];
#ifdef __cplusplus
extern "C" {
#endif
void get_DAFU_measure(Big_Fu_info_t *ptr, uint8_t can_rx_data[8]);     
void First_Board_receive(uint32_t canId,uint8_t  *rxBuf,Big_Fu_info_t *Big_Fu_info,Board_info_t Board_info);
void First_Board_Send(CAN_HandleTypeDef *hcan, uint32_t Id, uint8_t *msg, uint16_t len);
void Board_Info_Tx(void);
void CAN_Process_Data(uint8_t Rx_data[8]);
#ifdef __cplusplus
}
#endif
#endif
