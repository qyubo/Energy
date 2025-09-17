#pragma once
#ifndef SERVICE_REMOTER_H
#define SERVICE_REMOTER_H
#include "tx_api.h"
#include "main.h"

/**
 * @def RC_CH_VALUE_MIN
 * @brief 遥控器通道值的最小值
 */
#define RC_CH_VALUE_MIN 364

/**
 * @def RC_CH_VALUE_OFFSET
 * @brief 遥控器通道值的中间值
 */
#define RC_CH_VALUE_OFFSET 1024

/**
 * @def RC_CH_VALUE_MAX
 * @brief 遥控器通道值的最大值
 */
#define RC_CH_VALUE_MAX 1684

/**
 * @def RC_CH_OFFSET_MAX
 * @brief 遥控器通道最大偏移值
 */
#define RC_CH_OFFSET_MAX 660

/**
 * @def MOUSE_OFFSET_MAX
 * @brief 鼠标最大偏移值
 */
#define MOUSE_OFFSET_MAX 32767

/**
 * @def DR16_DATA_SIZE
 * @brief Dr16遥控器数据包的大小
 */
#define DR16_DATA_SIZE 18u

/**
 * @def VT03_DATA_SIZE
 * @brief VT03遥控器数据包的大小
 */
#define VT03_DATA_SIZE 21u

/**
 * @brief 遥控器数据结构。
 */
struct dr16_data_t
{
    uint16_t ch_0 : 11;
    uint16_t ch_1 : 11;
    uint16_t ch_2 : 11;
    uint16_t ch_3 : 11;
    uint16_t s1 : 2;
    uint16_t s2 : 2;

    int16_t mouse_x : 16;
    int16_t mouse_y : 16;
    int16_t mouse_z : 16;
    uint8_t mouse_left : 8;
    uint8_t mouse_right : 8;
    __PACKED_STRUCT
    {
        uint16_t W : 1;
        uint16_t S : 1;
        uint16_t A : 1;
        uint16_t D : 1;
        uint16_t SHIFT : 1;
        uint16_t CTRL : 1;
        uint16_t Q : 1;
        uint16_t E : 1;
        uint16_t R : 1;
        uint16_t F : 1;
        uint16_t G : 1;
        uint16_t Z : 1;
        uint16_t X : 1;
        uint16_t C : 1;
        uint16_t V : 1;
        uint16_t B : 1;
    }key;
    uint16_t reserve : 16;
} __attribute__((packed));

struct vt03_data_t
{
    uint8_t sof_1;
    uint8_t sof_2;
    uint64_t ch_0 : 11;
    uint64_t ch_1 : 11;
    uint64_t ch_2 : 11;
    uint64_t ch_3 : 11;
    uint64_t mode_sw : 2;
    uint64_t pause : 1;
    uint64_t fn_1 : 1;
    uint64_t fn_2 : 1;
    uint64_t wheel : 11;
    uint64_t trigger : 1;

    int16_t mouse_x;
    int16_t mouse_y;
    int16_t mouse_z;
    uint8_t mouse_left : 2;
    uint8_t mouse_right : 2;
    uint8_t mouse_middle : 2;
    __PACKED_STRUCT
    {
        uint16_t W : 1;
        uint16_t S : 1;
        uint16_t A : 1;
        uint16_t D : 1;
        uint16_t SHIFT : 1;
        uint16_t CTRL : 1;
        uint16_t Q : 1;
        uint16_t E : 1;
        uint16_t R : 1;
        uint16_t F : 1;
        uint16_t G : 1;
        uint16_t Z : 1;
        uint16_t X : 1;
        uint16_t C : 1;
        uint16_t V : 1;
        uint16_t B : 1;
    }key;
    uint16_t crc16;
} __attribute__((packed));

#endif