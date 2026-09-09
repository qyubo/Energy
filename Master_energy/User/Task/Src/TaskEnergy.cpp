//
// Created by cosmosmount on 2025/9/10.
//
// 功能说明：能量机关主板任务
// 【当前启用】上电自动模式：以 DWT 微秒级时间轴作为随机种子，从 3 块从板中随机抽出 1 块作为"不亮"的板：
//   - 其余 2 块发送 BOARD_DISPLAY_TARGET_ARROW(11)：继电器吸合亮靶面 + 红色箭头流水灯；
//   - 被抽中的板发送 BOARD_DISPLAY_BLACK_HALF_RED(12)：靶面全黑 + 箭头流水灯左半红；
//   - 未接从板的 ID 发送 BOARD_DISPLAY_OFF(0)，保持全黑。
//   主循环仅周期广播 CAN 指令；命中判定/游戏流程逻辑暂不实现。
// 【暂时注释保留】原遥控器控制模式（小符/大符/命中流程），需要时取消注释恢复，
//   恢复步骤见文件末尾注释块说明。
//

#include "bsp_dwt.hpp"
#include "main.h"
#include "tx_api.h"

#include "can_portocal_first.hpp"//
#include "bsp_can.hpp"//
#include <cstdint>
#include <stdlib.h>

//用到的库（原遥控器模式，暂不使用）
// #include "om.h"
// #include "pid.hpp"
// #include "math.hpp"
// #include "filter.hpp"
// #include "magicmsgs.hpp"
// #include "bsp.hpp"//
// #include "ServiceRemoter.hpp"
// #include "ServiceMotor.hpp"

// using namespace Filter;
// using namespace Numeric;
// extern DJIMotorHandler* DJIMotorhandler;

// 参与随机选靶的从板数量（对应 CAN ID 0x301~0x303，即 1~3 号板）
#define ENERGY_BOARD_NUM 3

TX_THREAD EnergyThread;
uint8_t EnergyThreadStack[4096] = {0};
// extern TX_SEMAPHORE EnergySem;//原遥控器模式信号量等待用，暂不使用

Board_Order_info_t Board_Order_info[5];//给从板发送的状态量
Big_Fu_info_t Big_Fu_info[5];//给主板发送信息
// Board_info_t Board_info;//五个扇叶待击打数量,无用
// uint8_t rxBuf[8]={0};//没用
// int Buff_time=0;
// uint8_t target_order[5]={1,4,0,3,2};//能量机关伪随机数
// volatile uint32_t msTicks = 0;

// void shuffle_order(uint8_t* array, uint8_t size){
//   for(int i =size-1;i>0;i--){
//     int j =rand()%(i+1);
//     uint8_t temp=array[i];
//     array[i]=array[j];
//     array[j]=temp;
//   }
// }//可能需要的随机函数（没测过
//转速结构体
// struct RunMotor_t {
//     float target_speed_rad; // 当前目标速度 (rad/s)
//     // 大符正弦运动参数
//     float a;
//     float w; // omega
//     float b;
//     uint32_t spin_start_time; // 进入激活状态的时间戳
//     bool params_generated;    // 是否已经生成过参数
// };
// static RunMotor_t run_motor = {0.0f, 0.0f, 0.0f, 0.0f, 0, false};

//定义随机函数
// float Rand_Float(float min, float max) {
//     float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
//     return min + scale * (max - min);      /* [min, max] */
// }
// //重置大能量机关参数函数
// void Reset_Rune_Motion_Params(void) {
//     run_motor.a = Rand_Float(0.780f, 1.045f);
//     run_motor.w = Rand_Float(1.884f, 2.000f);
//     run_motor.b = 2.090f - run_motor.a;
//     run_motor.spin_start_time = DWT_GetTimeline_ms(); // 记录 t=0 时刻
//     run_motor.params_generated = true;
// }

//声明topic（原遥控器模式，暂不使用）
// extern om_topic_t* remoter_topic;
// msg_remoter_t rc_debug;

/* ============================================================
 * 【当前启用】上电自动模式：随机选 2 块板亮靶面+箭头，其余全黑+半红
 * ============================================================ */
[[noreturn]] void EnergyThreadFun(ULONG initial_input)
{
    UNUSED(initial_input);

    // 上电随机选靶：DWT 微秒时间轴做种子，每次上电结果不同
    srand((unsigned)DWT_GetTimeline_us());
    // 从 3 块板中随机抽出 1 块"不亮"的板，其余 2 块亮靶面+箭头
    uint8_t dark_board = (uint8_t)(rand() % ENERGY_BOARD_NUM);

    for(int i = 0; i < 5; i++)
    {
        if(i < ENERGY_BOARD_NUM)
        {
            Board_Order_info[i].Board_Work =
                (i == dark_board) ? BOARD_DISPLAY_BLACK_HALF_RED
                                  : BOARD_DISPLAY_TARGET_ARROW;
        }
        else
        {
            Board_Order_info[i].Board_Work = BOARD_DISPLAY_OFF;//未接从板的 ID 保持全黑
        }
    }

    static uint32_t last_tx_time = 0;
    for(;;)
    {
        //HINT：5ms 发一次，经测量 1~2ms 发一次会有丢包
        uint32_t now_time = (uint32_t)DWT_GetTimeline_ms();
        if(now_time - last_tx_time >= 5)
        {
            Board_Info_Tx();
            last_tx_time = now_time;
        }
        tx_thread_sleep(1);
    }
}

/* ============================================================
 * 【暂时注释保留】原遥控器控制模式（小符/大符/命中流程）
 * 恢复方法：
 *   1. 取消本函数整体注释；
 *   2. 取消上方相关 include（om/pid/math/filter/magicmsgs/bsp/ServiceRemoter/ServiceMotor）、
 *      extern EnergySem、target_order、shuffle_order、rc_debug、remoter_topic 的注释；
 *   3. 与【当前启用】函数二选一：本函数恢复时已改名为 EnergyThreadFun_old，
 *      改回 EnergyThreadFun 并注释掉上电自动模式函数即可；
 *   4. 无需动线程创建：AZURE_RTOS/App/app_azure_rtos.c 的 tx_application_define
 *      已调用 ServiceBooster()/TaskBooster()（注意 app_threadx.c 里的调用是注释状态，勿重复添加）。
 * ============================================================ */
// [[noreturn]] void EnergyThreadFun_old(ULONG initial_input)
// {
//     UNUSED(initial_input);
//     // ServiceMotors::Instance()->MotorRegister();
//     // ServiceMotors::Instance()->SetModeAndPidParam();
//
// while(remoter_topic==nullptr){
//     tx_thread_sleep(1000);
// }
// om_suber_t* remoter_control=om_subscribe(remoter_topic);
// msg_remoter_t rc;
//
// //当前目标索引
// static int current_round_index=0;
// //游戏是否结束
// static bool game_over=true;
// //记录上次动作时间
// static uint32_t last_action_time = 0;
// //记录开始展示时间
// static uint32_t hit_show_start_time=0;
// //是否正在展示
// static bool is_showing_hit=false;
// //改变顺序
// // shuffle_order(target_order,5);//TODO:是否需要随机数
// static uint32_t last_tx_time=0;
// for(;;)
// {
//
//     if(tx_semaphore_get(&EnergySem,TX_WAIT_FOREVER)==TX_SUCCESS)
//     {
//         om_suber_export(remoter_control,&rc,false);
//         memcpy(&rc_debug,&rc,sizeof(msg_remoter_t));
//         //每次循环获得当前时间
//         int32_t now_time = DWT_GetTimeline_ms();
//         if(rc.offline==true||rc.ctrl_sw==Relax)//拨杆下初始化
//         {
//           game_over=true;
//           current_round_index=0;//初始化标志
//           for(int i=0;i<5;i++)
//           {
//             Board_Order_info[i].Board_Work=0;
//             Big_Fu_info[i].Single_Hit_State=0;//刷新上一次留下的数据
//           }
//           // 【电机控制】非激活状态，固定低速
//             // run_motor.target_speed_rad = Pi/3;
//             // run_motor.params_generated = false; // 清除标志，为下次Spin做准备
//             msTicks=0;
//
//
//         }
//         else if(rc.ctrl_sw==Normal)//拨杆中小符
//         {
//           // 【电机控制】非激活状态，固定低速
//           // run_motor.target_speed_rad = Pi/3;
//           // run_motor.params_generated = false;
//           // ServiceMotors::Instance()->FuMotor.speedSet=run_motor.target_speed_rad;
//           if(game_over)
//           {
//             game_over=false;
//             current_round_index=0;
//             last_action_time=now_time;
//
//             for(int i=0;i<5;i++)
//             {
//               Board_Order_info[i].Board_Work=0;
//               Big_Fu_info[i].Single_Hit_State=0;//刷新上一次留下的数据
//             }
//           }
//           if(current_round_index<5)
//         {
//           uint8_t target_id=target_order[current_round_index];
//           if(is_showing_hit)
//           {
//             if(now_time - hit_show_start_time >= 10) // 10ms防止误判
//         {
//             is_showing_hit = false;
//             current_round_index++;
//             last_action_time = now_time;
//
//             // 清楚数据
//             for(int i = 0; i < 5; i++) {
//                 Big_Fu_info[i].Single_Hit_State = 0;
//             }
//         }
//           }else {
//           if(Board_Order_info[target_id].Board_Work != 10)
//         {
//             Board_Order_info[target_id].Board_Work = 11;
//         }
//         if(Big_Fu_info[target_id].Single_Hit_State == 1)
//         {
//             Board_Order_info[target_id].Board_Work = 10; // 击打成功，亮起
//             Big_Fu_info[target_id].Single_Hit_State = 0; // 随手清零
//             is_showing_hit = true;
//             hit_show_start_time = now_time;
//         }
//         else if(now_time - last_action_time > 1000
//             || Big_Fu_info[(target_id+1)%5].Single_Hit_State == 1
//             || Big_Fu_info[(target_id+2)%5].Single_Hit_State == 1
//             || Big_Fu_info[(target_id+3)%5].Single_Hit_State == 1
//             || Big_Fu_info[(target_id+4)%5].Single_Hit_State == 1)//超时与误击打检测
//         {
//             game_over = true;
//         }
//           }
//
//         }
//         }
//
//         else if(rc.ctrl_sw==Spin)//拨杆上大符
//         {
//           if(game_over)
//           {
//             game_over=false;
//             current_round_index=0;
//             last_action_time=now_time;
//             is_showing_hit=false;
//             // run_motor.params_generated = false; // 清除标志，为下次Spin做准备
//             for(int i=0;i<5;i++)
//             {
//               Board_Order_info[i].Board_Work=0;
//               Big_Fu_info[i].Single_Hit_State=0;//刷新上一次留下的数据
//             }
//           }
//           // if(!run_motor.params_generated)
//           // {
//           //   Reset_Rune_Motion_Params();
//           //   run_motor.params_generated = true;
//           // }
//           // run_motor.target_speed_rad = run_motor.a * sinf(run_motor.w * (now_time - run_motor.spin_start_time)/1000) + run_motor.b;
//           if(current_round_index<5)
//           {
//           uint8_t target_id_1=target_order[current_round_index];
//           uint8_t target_id_2=target_order[(current_round_index+2)%5];//跳位取模（其实没必要）
//             if(is_showing_hit)//击打成功展示一秒
//             {
//               if(now_time - hit_show_start_time>=1000)
//               {
//                 Board_Order_info[target_id_1].Board_Work = 0;//复位
//                 Board_Order_info[target_id_2].Board_Work = 0;//复位
//                 current_round_index++;      // 晋级
//                 last_action_time = now_time;// 刷新超时计时器
//                 is_showing_hit=false;
//
//               }
//             }
//             else{
//           if(Board_Order_info[target_id_1].Board_Work<1 || Board_Order_info[target_id_1].Board_Work>10){Board_Order_info[target_id_1].Board_Work=11;}
//           if(Board_Order_info[target_id_2].Board_Work<1 || Board_Order_info[target_id_2].Board_Work>10){Board_Order_info[target_id_2].Board_Work=11;}//没被打就一直待命
//           bool hit_any=false;//记录是否击打成功
//               if(rc.shoot_sw==Closed)
//               {
//             Board_Order_info[target_id_1].Board_Work = 10; //击打成功
//             Big_Fu_info[target_id_1].Single_Hit_State = 0; //击打成功后清零状态
//             hit_any=true;
//               }
//               if(rc.shoot_sw==Fire)
//               {
//             Board_Order_info[target_id_2].Board_Work = 10; //击打成功
//             Big_Fu_info[target_id_2].Single_Hit_State = 0; //击打成功后清零状态
//             hit_any=true;
//               }
//           if(Big_Fu_info[target_id_1].Single_Hit_State == 1)
//           {
//             Board_Order_info[target_id_1].Board_Work = Big_Fu_info[target_id_1].Hit_LED_state; //击打成功
//             Big_Fu_info[target_id_1].Single_Hit_State = 0; //击打成功后清零状态
//             hit_any=true;
//           }else if(Big_Fu_info[target_id_2].Single_Hit_State == 1)
//           {
//             Board_Order_info[target_id_2].Board_Work = Big_Fu_info[target_id_2].Hit_LED_state; //击打成功
//             Big_Fu_info[target_id_2].Single_Hit_State = 0; //击打成功后清零状态
//             hit_any=true;
//           }
//           if(hit_any)//击打中进入展示
//           {
//             is_showing_hit=true;
//             hit_show_start_time=now_time;
//           }
//           else if(now_time - last_action_time > 2500 || Big_Fu_info[(target_id_1+1)%5].Single_Hit_State == 1
//         ||Big_Fu_info[(target_id_2+1)%5].Single_Hit_State == 1||Big_Fu_info[(target_id_2+2)%5].Single_Hit_State == 1)//HINT加入了击打错误结束
//           {
//             game_over=true;
//           }
//         }
//     }
//   }
//   // ServiceMotors::Instance()->FuMotor.speedSet=run_motor.target_speed_rad;
//   // DJIMotorhandler->sendControlData();
//   // Board_Info_Tx();
//   if (now_time - last_tx_time >= 5)
// {
//     Board_Info_Tx();
//     last_tx_time = now_time;
// }//HINT：5ms发一次，经测量1，2ms发一次会有丢包
//
//
//     }
//   tx_thread_sleep(1);
//
// }
// }
