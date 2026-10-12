#include "hit_detect.h"

#define BASELINE_FRAMES 1024  // 用1024帧计算初始基线
#define HIT_TRIGGER_THRESHOLD 1000
#define HIT_RELEASE_THRESHOLD 500
#define HIT_COLLECT_FRAMES 30

static const uint8_t ring_adc_channel[CHANNEL_COUNT] = {
    5, 1, 9, 3, 8, 4, 0, 2, 6, 7
};

uint16_t hit_baseline[CHANNEL_COUNT] = {0}; 
// 保存最终平均值，每路一个
static uint32_t baseline_sum[CHANNEL_COUNT] = {0};
static uint16_t baseline_count = 0;
static uint8_t baseline_ready = 0;
uint16_t hit_block_peak[CHANNEL_COUNT] = {0};  // 保存每个块的峰值，每路一个
uint16_t hit_event_peak[CHANNEL_COUNT] = {0};  // 保存每个击打事件的峰值，每路一个
static uint8_t hit_collecting = 0;  // 1表示正在收集击打事件，0表示未收集
static uint8_t hit_armed = 1;  // 1表示可以开始收集击打事件，0表示正在收集或已收集完
static uint16_t hit_collect_count = 0;
static uint8_t hit_pending_ring = 0;  // 保存等待主程序取走的识别结果，0表示没有击打事件


// 返回1表示基线已准备好，可以进行击打检测
uint8_t HitDetect_IsReady(void) {
    return baseline_ready;
}  

void HitDetect_Process(const uint16_t block[FRAMES_PER_BLOCK][CHANNEL_COUNT], uint8_t continuous) {
    // compute baseline
    if (baseline_ready == 0) {
        for (uint16_t frame_index = 0; frame_index < FRAMES_PER_BLOCK; frame_index++) {
            for (uint8_t ring = 0; ring < CHANNEL_COUNT; ring++) {
                uint8_t channel = ring_adc_channel[ring];
                baseline_sum[ring] += block[frame_index][channel];
            }
            baseline_count++;
            if (baseline_count == BASELINE_FRAMES) {
                for (uint8_t ring = 0; ring < CHANNEL_COUNT; ring++) {
                    hit_baseline[ring] = baseline_sum[ring] / BASELINE_FRAMES;
                }
                baseline_ready = 1;
                return;
            }
        }
        return;
    }
    // 清空峰值数组
    for (uint8_t ring = 0; ring < CHANNEL_COUNT; ring++) {
        hit_block_peak[ring] = 0;
    }
    // 遍历块中的每一帧，计算每个环的峰值
    for (uint16_t frame_index = 0; frame_index < FRAMES_PER_BLOCK; frame_index++) {
        for (uint8_t ring = 0; ring < CHANNEL_COUNT; ring++) {
            uint8_t channel = ring_adc_channel[ring];
            uint16_t value = block[frame_index][channel];
            if (value > hit_baseline[ring]) {
                uint16_t delta = value - hit_baseline[ring];
                if (delta > hit_block_peak[ring]) {
                    hit_block_peak[ring] = delta;
                }
            }
        }
    } 
    // 如果无数据（DMA一直循环写入，但主循环没有及时把已经完成的数据块复制出来，旧数据就被新数据覆盖了），放弃正在采集的击打
    if (continuous == 0) {
        hit_collecting = 0;
        hit_collect_count = 0;
        hit_armed = 0;
        return;
    }  
    // 如果当前没有采集击打，并且检测器还没重新准备好(上一次击打已经判断完，但传感器可能还在振动)，检查是否可以重新准备好
    if ((hit_collecting == 0) && (hit_armed == 0)) {  
        hit_armed = 1;
        for (uint8_t ring = 0; ring < CHANNEL_COUNT; ring++) {
            if (hit_block_peak[ring] >= HIT_RELEASE_THRESHOLD) {
                hit_armed = 0;
                break;  // 取消准备状态，并结束循环
            }
        }
        if (hit_armed == 0) {
            return;
        }
    }
    // 逐帧循环
    for (uint16_t frame = 0; frame < FRAMES_PER_BLOCK; frame++) {
        // 检查是否可以触发采集
        if ((hit_collecting == 0) && (hit_armed == 1)) {
            for (uint8_t ring = 0; ring < CHANNEL_COUNT; ring++) {
                uint8_t channel = ring_adc_channel[ring];
                uint16_t value = block[frame][channel];
                if ((value > hit_baseline[ring]) && (value - hit_baseline[ring] >= HIT_TRIGGER_THRESHOLD)) {
                    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
                        hit_event_peak[i] = 0;
                    }
                    hit_collecting = 1;
                    hit_collect_count = 0;
                    break;
                }
            }
        }
        // 没有触发采集就跳到下一帧
        if (hit_collecting == 0) {
            continue;
        }
        // 采集每个环的峰值
        for (uint8_t ring = 0; ring < CHANNEL_COUNT; ring++) {
            uint8_t channel = ring_adc_channel[ring];
            uint16_t value = block[frame][channel];
            if ((value > hit_baseline[ring]) && (value - hit_baseline[ring] > hit_event_peak[ring])) {
                hit_event_peak[ring] = value - hit_baseline[ring];
            }
        }
        hit_collect_count++;
        if (hit_collect_count >= HIT_COLLECT_FRAMES) {
            uint8_t winner = 0;
            for (uint8_t ring = 1; ring < CHANNEL_COUNT; ring++) {
                if (hit_event_peak[ring] > hit_event_peak[winner]) {
                    winner = ring;
                }
            }
            hit_pending_ring = winner + 1;  // 环号从1开始
            hit_collecting = 0;
            hit_armed = 0;
            break;
        }
    }
}  

uint8_t HitDetect_GetResult(uint8_t *ring) {
    if ((ring == 0) || (hit_pending_ring == 0)) {
        return 0; 
    }
    *ring = hit_pending_ring;
    hit_pending_ring = 0;
    return 1;
}