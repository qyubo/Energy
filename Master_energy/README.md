# RM26_F4
PnX26赛季电控F407IGHx开发板通用仓库。
## ToolChain
使用的工具链为CubeMX+CLion+Ozone，可以实现全平台开发、调试，在合适的系统支持下可以实现快速编译。
### CubeMX
用于配置各个外设，生成初始化代码，导出时勾选`STM32CubeIDE`选项，生成的代码可以直接导入CLion。

在配置时，注意`CmakeLists_template.txt`的更新与书写。
### CLion
用于代码编写，代码管理，代码编译，只需配置`arm-none-gebi`编译，可以选择STM32CLT，也可以安装原版，请自行搜索。

在Ubuntu下，`sudo`即可完成安装，无需CLT。
### Ozone
用于代码调试，烧录。拥有在线变量观看、曲线图可视化功能，需要搭配jlink使用（特定版本可能支持daplink）
### 教程
具体参照 `飞书-电控知识库`。
## ThreadX
### Intro
ThreadX 类似freeRTOS, 提供一款线程切换的操作系统API。他的产品涵盖了各种领域，
包括NASA的多个太空探测项目，飞机自动驾驶仪系统，火星侦察轨道器等。
总而言之，对比队伍之前使用的控制系统（主函数轮询配合带有中断优先级的中断处理流程实现多任务处理），
ThreadX系统不用考虑板子的中断优先级数量，且每个线程可以通过延时实现手动阻塞运行，
同时还能确保系统整体的实时性，是一款方便又好用的调度器。
### Basic usage
 首先需要明确下列名词的意思和用法：
- 线程
  - 运行任务的死循环
- 消息队列（本框架中使用下面的 `OneMessage` 代替消息队列）
    - 能够携带信号量+一定长度数据
    - 是“发布订阅”模型的一个实例，但是没有OneMessage好用
- 信号量
  - 只能携带一个数字（标志位）
  - **用于需要顺序执行的操作**：中断释放信号量，线程阻塞读取信号量
  - e.x.: 中断接收电机反馈信息，然后线程接收、处理数据
- 字节池
  - 使用内存池是为了避免DTCM（如DMA，RS485）
  - DTCM（Data Tightly Coupled Memory）：
    - 通常映射在 `0x20000000`（如 STM32H7 的 DTCM-RAM）
    - 高速、零等待访问
    - **不可被 DMA 控制器访问**（因为它不通过总线，而是 M7 内核直连）
  - AXI SRAM、SRAM1/2：
    - 位于 AHB 或 AXI 总线上
    - DMA 可读写
> ❗所以：DMA 要把缓冲区放在 DMA-accessible 区域，否则会造成不可预期的错误，比如数据无更新、总线 Fault、接收失败等
### 实质
> ThreadX 内存池分配的是你预先给定的大内存块（buffer）上的“定长子块”。
>
- `tx_block_pool_create()` 并不会去申请 malloc 之类的“堆”空间，而是**完全使用你给它的那段 buffer（`my_memory`）**。
- 所以你可以手动把 `my_memory` 放在 **DMA可访问区域**。

### Usage
简单使用方法需要掌握：

- 如何创建线程（线程内一定是死循环）
- 如何创建任务

详情见飞书教程链接

### 中断部分

- 遥控器数据获取
- can中断回调

### 线程部分

### 内存池

- uart (for receive buffer)
- KEF (for malloc)
- Msg (for OneMessage)

### 信号量

- CAN是否收到
- 遥控器数据是否收到

### thread

（优先级由大到小，即数字从小到大，2开始，7.8位一样，常规抢占式RTOS）
- MOTOR 2
- INS 3
- REMOTER 4
- IMU_Temperature 5 （与IMU分离，防止堵塞烧IMU）
- RobotCTRL 6（用于云台）
- Led(for test) 10
- to be continued

## Onemessage
### Intro
我们使用青岛大学开源的Onemessage进行各个模块与线程间的通信,包含如下六个功能模块：

- MSG 消息/话题控制: 控制消息的订阅与发布，话题和订阅者的创建与管理
- FMT 格式化配置：为用户提供格式化字符串的方式来快速配置话题/订阅/过滤器
- EVT 事件机制：基于topic实现的事件触发机制
- LOG 日志系统：多等级的日志打印
- AFL 高级过滤器：为topic提供灵活的筛选方式
- COM 通信解析：用于跨进程/跨设备共享topic

旨在为跨线程和跨设备通信提供一个通用解决方案，以较小的性能代价换取数倍的开发效率。

https://github.com/Jiu-xiao/OneMessage

### Usage
最简单使用方法如下：
创建与发布
```c++
om_topic_t *your_topic = om_config_topic(nullptr, "ca", "name", sizeof(msg_struct_t));
msg_struct_t msg;
...
for(;;){
    om_publish(your_topic, &msg, sizeof(msg), true, false);
}
```
订阅
```c++
om_suber_t *your_suber = om_subscribe(om_find_topic("name", UINT32_MAX));
msg_struct_t msg;
...
for(;;){
    om_suber_export(your_suber, &msg, false);
}
...
```
> 注意：如果订阅不存在话题，可能导致线程卡死。

## Structure Design
用户开发的代码存放在`User`文件夹下，与系统代码分离，方便管理。
```angular2html
User
├─BSP
│  ├─Inc
│  └─Src
├─Module
│  ├─AHRS
│  │  ├─Inc
│  │  └─Src
│  ├─DJIMotor
│  │  ├─Inc
│  │  └─Src
│  ├─DMMotor
│  │  ├─Inc
│  │  └─Src
│  ├─IMU
│  │  ├─Inc
│  │  └─Src
│  ├─LED
│  ├─LKMotor
│  │  ├─Inc
│  │  └─Src
│  ├─Logger
│  ├─OneMessage
│  │  ├─app
│  │  ├─comp
│  │  └─core
│  ├─Referee
│  ├─RemoteControl
│  │  ├─Inc
│  │  └─Src
│  └─SuperCap
├─Service
│  ├─Inc
│  └─Src
├─Task
│  ├─Inc
│  └─Src
└─Utils
├─Inc
└─Src
```
对不同分支下主要需要开发的应当只有`Service`和`Task`。
### Service
在该文件夹下，开发Motor、IMU、遥控器等较为底层的模块，为上层控制提供运行基础。在不同分支下，除了Motor部分，基本不需要进行其它修改。

如果有优化一般均为通用优化。
### Task
在该文件夹下，需在不同分支下独立开发，包含对本机器人的所有控制任务。

## Workflow
在搭建架构的同时，工作流上采用规范注释和单仓库多分支开发的方式，方便协作、版本管理并提高代码可读性、可移植性。
### Doxygen
语法：
```c++
    /**
     * @brief 定义电机测量数据的结构体。
     * @param last_ecd 上次电机编码器的读数。
     * @param ecd 当前电机编码器的读数。
     * @param speed_rpm 电机的转速，单位为rpm。
     * @param given_current 给定的电机电流，单位为毫安。
     * @param temperate 电机的温度，单位为摄氏度。
     */
 ```
在CLion和VSCode中均有相关高亮和语法提示，在书写新函数时请添加相关注释。
### Cherrypick
这种提交方法可以实现不同仓库的某个修改后的文件夹同步 ，
允许你从一个分支中选择某些提交（commit）并将它们应用到当前分支。
这对于想要在不合并整个分支的情况下将特定更改移植到其他分支时非常有用。
#### 基本用法
选择一个提交
如果你只想将一个特定的提交从其他分支合并到当前分支，可以使用以下命令：
```githubexpressionlanguage
git cherry-pick <commit-hash>
```
其中 <commit-hash> 是你想要挑选的提交的哈希值。例如：
```githubexpressionlanguage
git cherry-pick abc123
```

