#include "bsp_can.hpp"
#include "main.h"
#include "string.h"
#include "can_portocal_second.hpp"
/*
对于C板，有两个can。
*/
extern CAN_HandleTypeDef hcan;
//extern CAN_HandleTypeDef hcan2;
extern Board_Order_info_t Board_Order_info[5];

/**
 * @brief 初始化CAN滤波器配置。
 * 设置CAN硬件的滤波器，用于优化接收数据的处理。
 * 更多信息，请参考原文，链接：https://blog.csdn.net/weixin_54448108/article/details/128570593
 */
void CAN_Init(void)
{
    CAN_FilterTypeDef can_filter_st;                   ///< 定义过滤器结构体
    can_filter_st.FilterActivation = ENABLE;           ///< ENABLE使能过滤器
    can_filter_st.FilterMode = CAN_FILTERMODE_IDMASK;  ///< 设置过滤器模式--标识符屏蔽位模式
    can_filter_st.FilterScale = CAN_FILTERSCALE_32BIT; ///< 过滤器的位宽 32 位
    can_filter_st.FilterIdHigh = 0x0000;               ///< ID高位
    can_filter_st.FilterIdLow = 0x0000;                ///< ID低位
    can_filter_st.FilterMaskIdHigh = 0x0000;           ///< 过滤器掩码高位
    can_filter_st.FilterMaskIdLow = 0x0000;            ///< 过滤器掩码低位

    can_filter_st.FilterBank = 0;                                      ///< 过滤器组-双CAN可指定0~27
    can_filter_st.FilterFIFOAssignment = CAN_RX_FIFO0;                 ///< 与过滤器组管理的 FIFO
    HAL_CAN_ConfigFilter(&hcan, &can_filter_st);                      ///< HAL库配置过滤器函数
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING); //改了这里！/< 使能CAN的各种中断
    HAL_CAN_ActivateNotification(&hcan,CAN_IT_TX_MAILBOX_EMPTY);
     if(HAL_CAN_Start(&hcan) != HAL_OK)
     {
        Error_Handler();
         // 启动CAN失败的处理
     }
    if(HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        Error_Handler();
    }
    if(HAL_CAN_ActivateNotification(&hcan, CAN_IT_TX_MAILBOX_EMPTY) != HAL_OK)
    {
        Error_Handler();
    }
//    can_filter_st.SlaveStartFilterBank = 14;                           ///< 双CAN模式下规定CAN的主从模式的过滤器分配，从过滤器为14
//    can_filter_st.FilterBank = 14;                                     ///< 过滤器组-双CAN可指定0~27
    //HAL_CAN_ConfigFilter(&hcan2, &can_filter_st);                      ///< HAL库配置过滤器函数
    //HAL_CAN_Start(&hcan2);                                             ///< 使能CAN2控制器
    //HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING); ///< 使能CAN的各种中断
	
}

void CAN_Transmit(CAN_HandleTypeDef *hcan, uint32_t Id, uint8_t *msg, uint16_t len)
{
    if (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0)
    {
        // 如果邮箱已满，可以选择等待或者采取其他操作
        // 例如，可以添加一个超时机制或者直接返回错误状态
        // Monitor::instance()->Log_Messages(Monitor::WARNING, (uint8_t *)"CAN Tx Mailbox is full\r\n");
        return;
    }
    CAN_TxHeaderTypeDef tx_header; ///< 定义发送数据结构体
    uint32_t send_mail_box;        ///< 定义发送邮箱
    tx_header.StdId = Id;          ///< 标准ID
    tx_header.IDE = CAN_ID_STD;    ///< 标准帧
    tx_header.RTR = CAN_RTR_DATA;  ///< 远程传输请求
    tx_header.DLC = len;           ///< 数据长度
   HAL_StatusTypeDef status= HAL_CAN_AddTxMessage(hcan, &tx_header, msg, &send_mail_box);
		if(status!=HAL_OK)
		{
            return;
        }
}

/**
 * @todo 待完成整个函数
 * @brief 更新从CAN接收的电机数据。
 * @param index 电机的索引，用于识别特定的电机。
 * @param rx_header 指向接收到的数据的指针。
 * @param can_receive_data 指向电机测量数据结构的指针，用于存储更新的数据。
 */
void CAN_Receive(uint8_t Rx_data[8],uint32_t Id)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    // uint32_t fifo_level = HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0);
    HAL_StatusTypeDef status = HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx_header, rx_data);
   if(rx_header.StdId==Id)
	{	
    if(status==HAL_OK)
			{
        for (int i = 0; i < 8; i++)
        {
            Rx_data[i] = rx_data[i];
        }
			}
	}
}
// void CAN_Receive(uint8_t Rx_data[8],uint32_t Target_ID)
// {
//     CAN_RxHeaderTypeDef rx_header;
//     uint8_t rx_data[8];

//     // 1. 发送请求 (你已经在别处发了)
//     // CAN_Transmit(...); 

//     // 2. 【关键】死等数据到来！
//     // 只要 FIFO 是空的，就一直在这里转圈，直到超时
//     uint32_t wait_start = HAL_GetTick();
//     while (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) == 0)
//     {
//         if (HAL_GetTick() - wait_start > 100) // 等100ms
//         {
//             // 超时了！说明 Loopback 没通 (硬件/波特率/Filter问题)
//             return; 
//         }
//     }

//     // 3. 确定有数据了，再读，这次一定能读到真的
//     if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK)
//     {
//         // 4. 打印或判断 ID
//         if (rx_header.StdId == Target_ID)
//         {
//             // 成功！收到正确数据
//         }
//         else
//         {
//             // 收到数据了，但 ID 不对 (rx_header.StdId 是多少？)
//         }
//     }
// }
/**
 * @brief CAN接收中断回调函数，所有反馈在can上的数据会在这里根据ID进行分类并处理。
 * @param hcan CAN句柄
 * @note 该函数用于处理CAN接收中断，根据ID分类处理接收到的数据。但是这中方法可能会在回调里浪费时间，因为这里的处理是阻塞的。考虑是否需要将数据存储到一个缓冲区，然后在主循环中处理。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
   CAN_RxHeaderTypeDef rx_header;
   // 接收数据
   uint8_t rx_data[8];
   if(hcan->Instance == CAN1)
   {
   HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);
   /* 只接收本板指令 ID：0x301~0x305 依次对应板号 0~4，随 BOARD_NUMBER 自动匹配 */
   if(rx_header.StdId == (CAN_BOARD_ID_1_F + BOARD_NUMBER))
   {
       get_Centrol_measure(&Board_Order_info[BOARD_NUMBER], rx_data);
   }
   /* 原逐板硬编码写法（注释保留），例如 2 号板：
   switch(rx_header.StdId)
{
	//  case 0x301:
	// 	 get_Centrol_measure(&Board_Order_info[0],rx_data);
	//  break;
	 case 0x302:
		 get_Centrol_measure(&Board_Order_info[1],rx_data);
	 break;
	//  case 0x303:
	// 	 get_Centrol_measure(&Board_Order_info[2],rx_data);
	//  break;
	//  case 0x304:
	// 	 get_Centrol_measure(&Board_Order_info[3],rx_data);
	//  break;
	//  case 0x305:
	// 	 get_Centrol_measure(&Board_Order_info[4],rx_data);
	//  break;
        default:
        break;
}//TODO:注释掉其他
   */
   }


}
