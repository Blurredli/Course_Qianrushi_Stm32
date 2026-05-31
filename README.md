# STM32 嵌入式课程项目

本仓库包含嵌入式课程的 STM32 项目工程。

## 项目结构

```
sy1/
├── sy1_1/    # 实验1：基础流水灯
├── sy1_2/    # 实验2：按键控制流水灯
└── sy1_3/    # 实验3：串口控制流水灯频率

sy2/
├── sy2-1/    # 实验1：FreeRTOS多任务串口控制LED
├── sy2-2/    # 实验2：按键任务通知控制流水灯
└── sy2-3/    # 实验3：串口状态显示
```

## sy1 - 基础实验

### sy1_3 - 串口控制流水灯

通过串口接收频率值，控制流水灯的闪烁速度。

- **硬件平台**: STM32F103VET6
- **主频**: 72 MHz
- **串口**: USART1 (PA9-TX, PA10-RX)
- **LED**: LED1(PA0), LED2(PA1), LED3(PA2)

**通信协议**: `S<频率值>\r\n`（例: `S100\r\n` = 100Hz）

## sy2 - FreeRTOS 多任务实验

### sy2-1 - 串口控制3个LED独立闪烁频率

4个任务 + 1个消息队列，通过串口独立控制3个LED的闪烁频率。

- **UartTask**: 接收串口命令，解析后发送到消息队列
- **LED1Task/LED2Task/LED3Task**: 从队列获取频率，控制对应LED

**通信协议**: `L<led>F<freq>\r\n`（例: `L1F100\r\n` = LED1 100Hz）

### sy2-2 - 按键任务通知控制流水灯

2个任务，通过任务通知机制实现按键控制流水灯。

| 任务     | 优先级 | 功能                                      |
|----------|--------|-------------------------------------------|
| KeyTask  | Normal | 轮询 KEY1/KEY2/KEY3，20ms消抖，下降沿检测后发送通知 |
| LEDTask  | Low    | 接收通知控制3个LED流水灯                    |

**按键功能**:
- **KEY1**: 切换流水方向（正向 LED1→LED2→LED3 / 反向 LED3→LED2→LED1）
- **KEY2**: 增加速率（1000ms 每次减 200ms，低于 100ms 时回到 1000ms）
- **KEY3**: 暂停/恢复切换

### sy2-3 - 串口状态显示

在 sy2-2 基础上新增 UartTask，LEDTask 状态变化时通过串口发送当前状态。

| 状态     | 串口显示                      |
|----------|-------------------------------|
| 正向运行 | `Forward running.....`        |
| 反向运行 | `Reverse running.....`        |
| 正向加速 | `Forward accelerating.....`   |
| 反向加速 | `Reverse accelerating.....`   |
| 暂停     | `Running paused`              |

**数据流**: 按键 → KeyTask → `xTaskNotify` → LEDTask（更新全局变量）→ `xTaskNotify` → UartTask（串口发送）

## 编译与烧录

1. 使用 STM32CubeMX 生成工程
2. 使用 Keil MDK 或 STM32CubeIDE 编译
3. 通过 ST-Link 烧录到开发板

## 串口调试

使用串口助手（如 Putty、MobaXterm）连接：
- 波特率: 115200
- 数据位: 8
- 停止位: 1
- 校验位: None
- 流控: None