# STM32F407VG 传感器监控系统

基于 STM32F407VG 核心板 + FreeRTOS 的多传感器数据采集与控制系统。

## 硬件配置

- **核心板**: STM32F407VG (ARM Cortex-M4F, 168MHz, 硬件 FPU)
- **操作系统**: FreeRTOS + CMSIS-RTOS2
- **通信接口**: UART1 (有线) / UART2 + HC05 (蓝牙)
- **Flash**: 1MB, **SRAM**: 192KB

## 外设列表

| 外设 | 接口 | 功能 |
|------|------|------|
| LCD 屏幕 (ILI9341) | SPI2 | 显示传感器数据 |
| HC05 蓝牙模块 | UART2 DMA 空闲中断 | 无线数据传输 |
| BME280 传感器 | I2C | 温度、湿度、气压采集 |
| 光敏电阻 | ADC1 | 环境光线强度采集 |
| PM2.5 传感器 | ADC1 | 颗粒物浓度采集 |
| TB6612 电机驱动 | TIM2 PWM | 直流电机/风扇控制 |

## GPIO 引脚分配

| 功能 | 引脚 | 复用/模式 | 说明 |
|------|------|----------|------|
| **I2C1_SCL** | PB6 | AF_OD | BME280 时钟 |
| **I2C1_SDA** | PB7 | AF_OD | BME280 数据 |
| **USART1_TX** | PA9 | AF_PP | 调试串口发送 |
| **USART1_RX** | PA10 | AF_PP | 调试串口接收 |
| **USART2_TX** | PA2 | AF_PP | HC05 蓝牙发送 |
| **USART2_RX** | PA3 | AF_PP | HC05 蓝牙接收 |
| **ADC1_IN10** | PC0 | Analog | 光敏电阻 |
| **ADC1_IN12** | PC2 | Analog | PM2.5 传感器 |
| **TIM2_CH3** | PB10 | AF_PP | 电机A PWM |
| **TIM2_CH4** | PB11 | AF_PP | 电机B PWM |
| **AIN1** | PA0 | Output | 电机A方向1 |
| **AIN2** | PA1 | Output | 电机A方向2 |
| **BIN1** | PB0 | Output | 电机B方向1 |
| **BIN2** | PB1 | Output | 电机B方向2 |
| **STBY** | PA5 | Output | 电机待机 |

| **SPI2_SCK** | **PB13** | AF_PP | LCD 时钟 (SPI2) |
| **SPI2_MOSI** | **PB15** | AF_PP | LCD 数据 (SPI2) |
| **LCD_CS** | **PD7** | Output | LCD 片选 |
| **LCD_DC** | **PD4** | Output | LCD 数据/命令 |
| **LCD_RST** | **PD5** | Output | LCD 复位

## 系统架构

```
┌───────────────────────────────────────────────────────────┐
│                       FreeRTOS                            │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  生产者（各自独立采集，只发原始数据消息）                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                │
│  │ BME280   │  │  Light   │  │   PM25   │                │
│  │  Task    │  │  Task    │  │   Task   │                │
│  └────┬─────┘  └────│─────┘  └────│─────┘                │
│       │        ┌────┴─────────────┘                       │
│       │        │  ADC1 (osMutex 保护)                     │
│       │        ▼                                          │
│       │  [sensorRawQueue] 消息队列                        │
│       ▼             ▼             ▼                       │
│  ┌──────────────────────────────────────────┐             │
│  │    解析任务 DistributorTask               │             │
│  │  接收消息 → 滤波 → 原子写入共享池 → 发标志  │             │
│  └──────┬──────────────┬─────────────┬──────┘             │
│         │              │             │                    │
│         ▼              ▼             ▼                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                │
│  │  LCD     │  │  UART    │  │   Fan    │                │
│  │  Task    │  │  Task    │  │   Task   │                │
│  │ 读全部   │  │ 读全部   │  │ 只读温度  │                │
│  └──────────┘  └──────────┘  └──────────┘                │
│      消费者（独立 EventFlags 唤醒，零拷贝读共享内存）        │
│                                                           │
│                        ┌───────┐                          │
│                        │  Cmd  │                          │
│                        │ Task  │                          │
│                        └───┬───┘                          │
│                            │                              │
│                  cmdQueue (命令队列)                        │
│                            │                              │
│                  ┌─────────┴─────────┐                    │
│                  ▼                   ▼                    │
│           ┌──────────────┐  ┌────────────────┐            │
│           │  MAVLink TX  │  │  MAVLink RX    │            │
│           │ (SendSensor  │  │ (ProcessByte → │            │
│           │  Data/Heart) │  │  on_mavlink_cb)│            │
│           └──────────────┘  └────────────────┘            │
│                                                           │
│  ┌──────────────┐                                         │
│  │ Monitor Task │──── IWDG 看门狗 (4s超时) ──→ 系统复位      │
│  └──────────────┘                                         │
│        │                                                  │
│        └── 定时检查所有 9 个任务的心跳                      │
└───────────────────────────────────────────────────────────┘
              │
    ┌─────────┴─────────┐
    │                   │
UART1 (有线)      UART2 (HC05 蓝牙)
    │                   │
    ▼                   ▼
Linux/PC          Linux/PC/Android
    (MAVLink)         (MAVLink)
```

详细见[系统架构图](系统架构图.png)

## 任务列表

| 任务名 | 优先级 | 栈大小 | 类型 | 功能 |
|--------|--------|--------|------|------|
| bme280Task | Normal | 1024B | 生产者 | 采集 BME280 温湿度气压 → 发送原始消息到队列 |
| lightTask | Normal | 1024B | 生产者 | 采集光敏 AD 值 → 发送原始消息到队列 (ADC Mutex) |
| pm25Task | Normal | 1024B | 生产者 | 采集 PM2.5 AD 值 → 发送原始消息到队列 (ADC Mutex) |
| **distributorTask** | **Normal** | **1024B** | **解析** | **接收原始消息 → 滑动平均滤波 → 原子写共享池 → 发 EventFlags** |
| lcdTask | BelowNormal | 1024B | 消费者 | 等待 lcdEventFlags → 从共享池零拷贝读全部 → LCD 显示 |
| uartTask | Normal | 1024B | 消费者 | 等待 uartEventFlags → 从共享池读全部 → MAVLink 发送 |
| fanTask | Normal | 1024B | 消费者 | 等待 fanEventFlags → 只读温度 → 状态机控制风扇 |
| cmdTask | High | 1024B | 处理 | 处理外部 MAVLink 命令 + OTA |
| monitorTask | Idle | 512B | 监控 | 系统监控 + 喂狗

## 设计要点

### 三层解耦架构：生产者 → 解析任务 → 消费者
- **第一层（生产者）**：BME280、光敏、PM2.5 各为独立任务，只负责硬件采集，采集完立即发送 `SensorRawMsg` 到消息队列，**不做滤波/写池/发标志**
- **第二层（解析任务）**：`DistributorTask` 接收所有原始消息 → 滑动平均滤波 → **原子写入共享内存池** → 统一发 EventFlags 唤醒消费者。BME280 的温/湿/压三个值在此同步写入，不会被消费者读到半新半旧的状态
- **第三层（消费者）**：LCD/UART/风扇通过独立的 `osEventFlags` 被唤醒，从共享内存池**零拷贝读取**
- 风扇只订阅温度事件，响应延迟从 500ms 降至 ~10ms
- 生产者故障隔离：某个传感器任务挂掉不影响其他传感器任务
- 新增传感器只需：加一个生产者任务（纯采集）+ 在解析任务加一个 case，消费者无需修改
- **ADC1 外设共享保护**：LightTask 和 PM25Task 共用 ADC1，通过 `osMutex` 互斥锁保证 ADC 通道切换和采样转换的原子性

### 滑动平均滤波器
- 每个生产者任务各自持有独立的滑动平均滤波器实例
- 8 点滑动平均，环形缓冲区 O(1) 更新

### 硬件看门狗 + 任务健康监控
- IWDG 独立看门狗，4s 超时自动复位
- Monitor Task 以 2s 周期喂狗并检查所有 **9** 个任务的心跳
- 临界任务（BME280、Cmd、Monitor）连续 3 次超时触发系统停机
- 非临界任务（LCD、UART、Fan、Light、PM25、Distributor）超时只计次不触发复位

### Flash 参数存储
- 风扇模式、校准值等系统参数保存到 Flash 末尾页
- CRC16 (Modbus) 校验数据完整性
- 写入前擦除整页，写入计数递增
- 系统上电自动恢复上次配置

### 风扇状态机
- 5 状态有限状态机控制风扇转速
- 10s 迟滞时间避免温度边界频繁切换
- 控制输入使用滤波后数据

### 硬件自检
- 接收 `CMD_DIAGNOSTIC` (MAVLink msgid=50004) 触发全系统自检
- 覆盖项：I2C、ADC、LCD、UART、电机、BME280、Flash
- 返回 MAVLink DIAG_REPORT 二进制数据

### 自动版本信息
- 编译时生成版本字符串（含日期时间）
- 接收 `CMD_GET_VERSION` (msgid=50007) 返回 VERSION_REPORT 帧

## 通信协议 — MAVLink v2

本项目使用 **MAVLink v2** 协议进行 STM32 与 Linux 之间的全双工通信。  
MAVLink 是一种轻量级二进制协议，内置帧同步、CRC16 校验和消息 ID 路由。

### 帧格式

```
┌──────┬──────┬──────┬──────┬──────┬──────┬──────┬────────┬──────────┬──────┬──────┐
│ 0xFD │ Len  │ Incomp│ Comp │ Seq  │ SysID│ Comp │ MsgID  │ Payload  │ CRC  │ CRC  │
│      │      │ Flags │ Flags│      │      │  ID  │ (24bit)│ (0-255B) │ 低   │ 高   │
└──────┴──────┴──────┴──────┴──────┴──────┴──────┴────────┴──────────┴──────┴──────┘
   1B      1B      1B      1B     1B     1B     1B      3B       0-255B     1B    1B
                                                          总帧头 = 10 字节
```

### 消息定义

| ID | 方向 | 消息名 | 载荷 | 说明 |
|----|------|--------|------|------|
| **0** | 双向 | **HEARTBEAT** | 9B | 心跳包（type, autopilot, base_mode, system_status） |
| **50000** | STM32→Linux | **SENSOR_DATA** | 25B | 传感器数据上报（temp, humid, press, light, pm25, ts, validity） |
| **50001** | Linux→STM32 | **CMD_MOTOR** | 4B | 设置电机速度（speed: int32） |
| **50002** | Linux→STM32 | **CMD_FAN** | 1B | 设置风扇模式（mode: 0=auto, 1=manual） |
| **50003** | STM32→Linux | **STATUS_REPORT** | 16B | 系统状态（uptime, fan_speed, fan_mode, task_alive_mask） |
| **50004** | STM32→Linux | **DIAG_REPORT** | 32B | 诊断报告（raw diagnostic data） |
| **50005** | Linux→STM32 | **CMD_GET_DATA** | 0B | 请求传感器数据 |
| **50006** | Linux→STM32 | **CMD_GET_STATUS** | 0B | 请求系统状态 |
| **50007** | Linux→STM32 | **CMD_GET_VERSION** | 0B | 请求固件版本 |
| **50008** | STM32→Linux | **VERSION_REPORT** | 32B | 版本号字符串 |

### Linux 端使用示例 (Python + pymavlink)

```python
from pymavlink import mavutil
import mavutil_enr  # 自定义消息定义

master = mavutil.mavlink_connection('/dev/ttyUSB0', baud=115200)

# 接收传感器数据
msg = master.recv_match(type='SENSOR_DATA', blocking=True)
print(f"温度: {msg.temperature:.2f}°C")
print(f"湿度: {msg.humidity:.1f}%")
print(f"气压: {msg.pressure:.1f}hPa")
print(f"光照: {msg.light:.0f}lx")
print(f"PM2.5: {msg.pm25:.1f}μg/m³")

# 获取系统状态
master.mav.cmd_get_status_send(1, 1)
msg = master.recv_match(type='STATUS_REPORT', blocking=True)
print(f"运行时间: {msg.uptime}ms")

# 设置风扇为自动模式
master.mav.cmd_fan_send(1, 1, 0)

# 设置电机速度 50%
master.mav.cmd_motor_send(1, 1, 50)
```

### 传感器数据帧格式 (SENSOR_DATA, ID=50000)

```
偏移   大小   字段         说明
─────────────────────────────────
 0     4     temperature   温度 (°C, float)
 4     4     humidity      湿度 (%RH, float)
 8     4     pressure      气压 (hPa, float)
12     4     light         光照 (lx, float)
16     4     pm25          PM2.5 (μg/m³, float)
20     4     timestamp     时间戳 (uint32)
24     1     validity      有效标志位
                    bit 0: BME280 有效
                    bit 1: 光敏有效
                    bit 2: PM2.5 有效
─────────────────────────────────
总计: 25 字节

## OTA 固件升级

通过蓝牙 (MAVLink v2) 无线升级固件，无需拆机接烧录器。

### Flash 布局

```
0x08000000  ┌────────────────────┐
            │ Bootloader (128KB)  │ Sector 0
0x08020000  ├────────────────────┤
            │ App (384KB)        │ Sectors 1~3
0x08080000  ├────────────────────┤
            │ OTA 下载区 (384KB)  │ Sectors 4~6
0x080E0000  ├────────────────────┤
            │ Storage 参数       │ Sector 7
0x08100000  └────────────────────┘

注: STM32F407VG Flash 为 1MB，最小擦除单位为 128KB 扇区
```

### 升级流程

```
Linux                              STM32
 │                                    │
 │ 1. OTA_BEGIN (total_size, CRC)  ──→│ 擦除下载区
 │  ←── OTA_ACK(status=OK) ────────── │
 │                                    │
 │ 2. OTA_DATA (offset=0, data[128])─→│ 写入 Flash
 │  ←── OTA_ACK(status=OK, written) ─ │
 │  OTA_DATA (offset=128) ───────────→│ ...
 │  ←── OTA_ACK ───────────────────── │
 │  ...                               │
 │                                    │
 │ 3. OTA_COMPLETE (CRC32) ──────────→│ 校验 → 写魔数
 │  ←── OTA_ACK(status=OK) ────────── │ NVIC_SystemReset()
 │                                    ▼
 │                            Bootloader 启动
 │                             检测到魔数 OTA_MAGIC
 │                             搬 OTA 区 → App 区
 │                             清除魔数
 │                             跳 App
 │                                    │
 │  ←── App 启动，发送 HEARTBEAT ──── │
```

### 使用方式

```bash
# 1. 编译生成固件（用 Keil 编译后）
fromelf --bin --output=firmware.bin Objects/stm32-enr.axf

# 2. 运行 OTA 升级脚本
python Tools/ota_update.py /dev/rfcomm0 firmware.bin

### Bootloader 项目

Bootloader 是**独立 Keil 项目**，位于 `Bootloader/` 目录：

| 文件 | 说明 |
|------|------|
| `Bootloader/Src/main.c` | 启动代码：时钟→检查 OTA→搬固件→跳 App |
| `Bootloader/bootloader.sct` | 链接脚本 (0x08000000, 128KB) |

**首次烧录步骤：**
1. 用 Keil 打开 `Bootloader/` 下新建的项目，添加 `Bootloader/Src/main.c`
2. 链接脚本使用 `Bootloader/bootloader.sct`
3. 编译烧录到 STM32（首次直接烧到 0x08000000）
4. 之后用 Keil 编译 App 项目（注意链接地址已改为 0x08020000）
5. 首次 App 烧录需通过 SWD，之后即可用 OTA 升级

## 风扇状态机

| 状态 | 转速 | 升迁条件 | 降级条件 |
|------|------|---------|---------|
| IDLE | 0% | 温度 > 28°C → LOW | - |
| LOW | 25% | 温度 > 32°C → MEDIUM | 温度 < 26°C 且 >= 10s → IDLE |
| MEDIUM | 50% | 温度 > 36°C → HIGH | 温度 < 30°C 且 >= 10s → LOW |
| HIGH | 75% | 温度 > 40°C → MAX | 温度 < 34°C 且 >= 10s → MEDIUM |
| MAX | 100% | - | 温度 < 38°C 且 >= 10s → HIGH |

## 数据格式

所有通信数据均以 **MAVLink v2 二进制帧** 格式传输，详见[通信协议](#-通信协议--mavlink-v2)章节。

不再使用 JSON 或纯文本格式。

## 项目结构

```
Core/
├── Inc/
│   ├── main.h              # 主头文件
│   ├── FreeRTOSConfig.h    # FreeRTOS 配置 (heap=15KB)
│   ├── sensor_data.h       # 传感器数据结构和命令定义
│   ├── sensor_raw_msg.h    # 生产者→解析任务的消息类型定义
│   ├── bme280.h            # BME280 驱动
│   ├── lcd.h               # LCD 驱动
│   ├── motor.h             # 电机驱动
│   ├── filter.h            # 数字滤波器
│   ├── watchdog.h          # 看门狗 + 任务监控 (9 任务)
│   ├── storage.h           # Flash 参数存储
│   ├── version.h           # 编译版本信息
│   ├── diagnostic.h        # 硬件自检框架
│   ├── usart.h             # 串口驱动 (含 HC05_SoftInit)
│   ├── mavlink_protocol.h  # MAVLink 协议封装层
│   └── mavlink/            # MAVLink v2 协议栈
│       ├── mavlink_types.h     # 核心类型定义
│       ├── checksum.h          # CRC16-CCITT 查表算法
│       ├── mavlink_helpers.h   # 帧编解码引擎
│       └── protocol.h          # 自定义消息定义 (14条)
└── Src/
    ├── main.c              # 主程序入口 (168MHz, FPU, VTOR)
    ├── freertos.c          # 9 个 FreeRTOS 任务 (3 生产者 + 1 解析 + 4 消费者 + 1 监控)
    ├── bme280.c            # BME280 驱动实现
    ├── lcd.c               # LCD 驱动 (含完整字模)
    ├── motor.c             # 电机驱动
    ├── filter.c            # 滤波器实现
    ├── watchdog.c          # 看门狗实现
    ├── storage.c           # Flash 存储实现
    ├── diagnostic.c        # 自检实现
    ├── usart.c             # UART DMA 空闲中断 + MAVLink + HC05 初始化
    ├── mavlink_protocol.c  # MAVLink 协议封装实现
    ├── ota.c               # OTA Flash 擦写
    ├── stm32f4xx_it.c      # 中断处理 (USART2, DMA Stream5/6)
    ├── stm32f4xx_hal_msp.c # HAL MSP 初始化
    └── system_stm32f4xx.c  # 系统时钟配置
```

## 编译

使用 Keil MDK (v5) ，编译并烧录到目标板。


## 注意事项

1. **HC05 蓝牙模块需配置为 115200 波特率**（AT 指令：`AT+UART=115200,0,0`）
2. UART1（调试）和 UART2（HC05）均使用 **115200** 波特率，误差 0.16%
3. LCD 使用 SPI2（PB13/PB15）5 线模式，控制引脚复用原 FSMC 引脚（PD4/5/7），模板现已无 FSMC 代码
4. 电机/风扇使用 TIM2 的 CH3 和 CH4 通道 (PB10/PB11)
5. BME280 使用 I2C1 接口 (PB6/PB7)
6. 光敏和 PM2.5 使用 ADC1 通道 10/12（PC0/PC2）
7. 系统参数存储在 Flash Sector 7 (地址 0x080E0000)，占用 128KB 扇区
8. 首次上电自动初始化默认参数并写入Flash
9. **通信协议已从 JSON 文本升级为 MAVLink v2 二进制协议**，旧版文本客户端不再兼容
10. **Bootloader + OTA 升级**：Flash 地址 0x08000000~0x0801FFFF 为 Bootloader，App 偏移到 0x08020000，通过蓝牙 MAVLink 接收 OTA 固件到 0x08080000，校验后重启进 Bootloader 搬移
11. **编译 App 后需用 `fromelf` 转为 .bin 文件再 OTA**，Keil 直接输出 .axf，需执行：`fromelf --bin --output=firmware.bin Objects/stm32-enr.axf`
