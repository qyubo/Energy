#include "usart.h"
// #include "bsp_usart.hpp"
#include "om.h"
#include "magicmsgs.hpp"
#include "ServiceRemoter.hpp"

TX_THREAD RemoterThread;
uint8_t RemoterThreadStack[2048] = {0};
TX_SEMAPHORE RemoterThreadSem;

// extern DMA_HandleTypeDef hdma_usart3_rx;
// extern uint8_t SBUS_rx_buf[2][SBUS_RX_BUF_NUM];

// 数组在 D3 RAM
uint8_t data_rx[DR16_DATA_SIZE];

inline dr16_data_t& Dr16_Data()
{
    return *reinterpret_cast<dr16_data_t*>(data_rx);
}

[[noreturn]] void RemoterThreadFun(ULONG initial_input) {
    UNUSED(initial_input);

    /* Remoter Topic */
    om_topic_t *remoter_topic = om_config_topic(nullptr, "ca", "remoter", sizeof(msg_remoter_t));
    msg_remoter_t msg_remoter{};
    // HAL_UART_Abort(&huart3);
    // HAL_UARTEx_ReceiveToIdle_DMA(&huart3, data_rx, DR16_DATA_SIZE);
    for (;;) {
        msg_remoter.offline = false;
        while (tx_semaphore_get(&RemoterThreadSem, 100) != TX_SUCCESS) {
            // 超时/掉线逻辑
            // 比如可以清零，或者标记掉线
            msg_remoter.offline = true;
            // HAL_UART_Abort(&huart3);
            // 可以加上状态标记，比如 rc_raw.online = false;
            tx_thread_sleep(3);
            // HAL_UARTEx_ReceiveToIdle_DMA(&huart3, data_rx, DR16_DATA_SIZE);
        }
        // 开关
        msg_remoter.ctrl_sw  = static_cast<CTRL_STATE>(Dr16_Data().s2);
        msg_remoter.shoot_sw = static_cast<SHOOT_STATE>(Dr16_Data().s1);

        if (msg_remoter.last_ctrl_sw == CTRL_STATE::Relax && msg_remoter.ctrl_sw == CTRL_STATE::Normal) {
            msg_remoter.ctrl_sw = CTRL_STATE::R2N;
        }
        else if (msg_remoter.last_ctrl_sw == CTRL_STATE::Normal && msg_remoter.ctrl_sw == CTRL_STATE::Relax) {
            msg_remoter.ctrl_sw = CTRL_STATE::N2R;
        }
        else if (msg_remoter.last_ctrl_sw == CTRL_STATE::Normal && msg_remoter.ctrl_sw == CTRL_STATE::Spin) {
            msg_remoter.ctrl_sw = CTRL_STATE::N2S;
        }
        else if (msg_remoter.last_ctrl_sw == CTRL_STATE::Spin && msg_remoter.ctrl_sw == CTRL_STATE::Normal) {
            msg_remoter.ctrl_sw = CTRL_STATE::S2N;
        }

        // 摇杆 11 位 -> float [-1,1]
        msg_remoter.right_x  = (static_cast<float>(Dr16_Data().ch_0) - RC_CH_VALUE_OFFSET) / RC_CH_OFFSET_MAX;
        msg_remoter.right_y  = (static_cast<float>(Dr16_Data().ch_1) - RC_CH_VALUE_OFFSET) / RC_CH_OFFSET_MAX;
        msg_remoter.left_x = (static_cast<float>(Dr16_Data().ch_2) - RC_CH_VALUE_OFFSET) / RC_CH_OFFSET_MAX;
        msg_remoter.left_y = (static_cast<float>(Dr16_Data().ch_3) - RC_CH_VALUE_OFFSET) / RC_CH_OFFSET_MAX;

        // 鼠标
        msg_remoter.mouse_x  = static_cast<float>(Dr16_Data().mouse_x);
        msg_remoter.mouse_y  = static_cast<float>(Dr16_Data().mouse_y);
        msg_remoter.mouse_z  = static_cast<float>(Dr16_Data().mouse_z);
        msg_remoter.mouse_left  = Dr16_Data().mouse_left != 0;
        msg_remoter.mouse_right = Dr16_Data().mouse_right != 0;

        // 键盘位域可以直接 memcpy
        memcpy(&msg_remoter.key, &Dr16_Data().key, sizeof(msg_remoter.key));
        om_publish(remoter_topic, &msg_remoter, sizeof(msg_remoter), true, false);
        msg_remoter.last_ctrl_sw = msg_remoter.ctrl_sw;
        msg_remoter.last_shoot_sw = msg_remoter.shoot_sw;
        memcpy(&msg_remoter.last_key, &msg_remoter.key, sizeof(msg_remoter.key));
        // HAL_UARTEx_ReceiveToIdle_DMA(&huart3, data_rx, DR16_DATA_SIZE);
        tx_thread_sleep(1);
    }
}

// void USART3_IRQHandler(void)
// {
//     /* USER CODE BEGIN USART3_IRQn 0 */
//
//     /* USER CODE END USART3_IRQn 0 */
//     HAL_UART_IRQHandler(&huart3);
//     /* USER CODE BEGIN USART3_IRQn 1 */
//     if (huart3.Instance->SR & UART_FLAG_RXNE)
//     {
//         __HAL_UART_CLEAR_PEFLAG(&huart3);
//     }
//     else if (USART3->SR & UART_FLAG_IDLE)
//     {
//         static uint16_t current_rx_len = 0;
//
//         __HAL_UART_CLEAR_PEFLAG(&huart3);
//
//         if ((hdma_usart3_rx.Instance->CR & DMA_SxCR_CT) == RESET)
//         {
//             /* Current memory buffer used is Memory 0 */
//
//             // disable DMA
//             __HAL_DMA_DISABLE(&hdma_usart3_rx);
//
//             // get receive data length, length = set_data_length - remain_length
//             current_rx_len = SBUS_RX_BUF_NUM - hdma_usart3_rx.Instance->NDTR;
//
//             // reset set_data_lenght
//             hdma_usart3_rx.Instance->NDTR = SBUS_RX_BUF_NUM;
//
//             // set memory buffer 1
//             hdma_usart3_rx.Instance->CR |= DMA_SxCR_CT;
//
//             // enable DMA
//             __HAL_DMA_ENABLE(&hdma_usart3_rx);
//
//             if (current_rx_len == DR16_DATA_SIZE)
//             {
//                 memcpy(data_rx, SBUS_rx_buf[0], DR16_DATA_SIZE);
//                 tx_semaphore_put(&RemoterThreadSem);
//             }
//         }
//         else
//         {
//             /* Current memory buffer used is Memory 1 */
//             // disable DMA
//             __HAL_DMA_DISABLE(&hdma_usart3_rx);
//
//             // get receive data length, length = set_data_length - remain_length
//             current_rx_len = SBUS_RX_BUF_NUM - hdma_usart3_rx.Instance->NDTR;
//
//             // reset set_data_lenght
//             hdma_usart3_rx.Instance->NDTR = SBUS_RX_BUF_NUM;
//
//             // set memory buffer 0
//             DMA1_Stream1->CR &= ~(DMA_SxCR_CT);
//
//             // enable DMA
//             __HAL_DMA_ENABLE(&hdma_usart3_rx);
//
//             if (current_rx_len == DR16_DATA_SIZE)
//             {
//                 memcpy(data_rx, SBUS_rx_buf[1], DR16_DATA_SIZE);
//                 tx_semaphore_put(&RemoterThreadSem);
//             }
//         }
//     }
//     /* USER CODE END USART3_IRQn 1 */
// }
