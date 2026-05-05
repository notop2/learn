# STM32F103VET6 传感器监控系统

基于 STM32F103VET6 核心板 + FreeRTOS 的多传感器数据采集与控制系统。

## 硬件配置

- **核心板**: STM32F103VET6 (ARM Cortex-M3)
- **操作系统**: FreeRTOS + CMSIS-RTOS2
- **通信接口**: UART1 (有线) / UART2 + HC05 (蓝牙)

## 外设列表

| 外设 | 接口 | 功能 |
|------|------|------|
| LCD 屏幕 (ILI9341) | FSMC | 显示传感器数据 |
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
| **TIM2_CH3** | PB10 | AF_PP | 电机 PWM (部分重映射) |
| **AIN1** | PA0 | Output | 电机A方向1 |
| **AIN2** | PA1 | Output | 电机A方向2 |
| **BIN1** | PB0 | Output | 电机B方向1 |
| **BIN2** | PB1 | Output | 电机B方向2 |
| **STBY** | PA5 | Output | 电机待机 |

### FSMC (LCD)

| 引脚 | 功能 | 引脚 | 功能 |
|------|------|------|------|
| PD0 | FSMC_D2 | PE7 | FSMC_D4 |
| PD1 | FSMC_D3 | PE8 | FSMC_D5 |
| PD4 | FSMC_NOE | PE9 | FSMC_D6 |
| PD5 | FSMC_NWE | PE10 | FSMC_D7 |
| PD7 | FSMC_NE1 | PE11 | FSMC_D8 |
| PD8 | FSMC_D13 | PE12 | FSMC_D9 |
| PD9 | FSMC_D14 | PE13 | FSMC_D10 |
| PD10 | FSMC_D15 | PE14 | FSMC_D11 |
| PD11 | FSMC_A16 | PE15 | FSMC_D12 |
| PD14 | FSMC_D0 |  |  |
| PD15 | FSMC_D1 |  |  |

## 系统架构

```
┌───────────────────────────────────────────────────────────┐
│                       FreeRTOS                            │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │ Sensor   │  │   LCD    │  │   UART   │  │   Fan    │  │
│  │  Task    │  │   Task   │  │   Task   │  │   Task   │  │
│  └────┬─────┘  └────▲─────┘  └────▲─────┘  └────┬─────┘  │
│       │             │             │             │         │
│       ▼             │             │             │         │
│  ┌──────────────────────────────────────────────┐         │
│  │              dataQueue (消息队列)              │         │
│  └──────────────────────────────────────────────┘         │
│                        ┌───────┐                          │
│                        │  Cmd  │                          │
│                        │ Task  │                          │
│                        └───┬───┘                          │
│                            │                              │
│                    cmdQueue (命令队列)                      │
│                            │                              │
│                  ┌─────────┴─────────┐                    │
│                  ▼                   ▼                    │
│           ┌──────────────┐  ┌────────────────┐            │
│           │   UART TX    │  │  Command Parse  │            │
│           └──────────────┘  └────────────────┘            │
│                                                           │
│  ┌──────────────┐                                         │
│  │ Monitor Task │──── IWDG 看门狗 (4s超时) ──→ 系统复位      │
│  └──────────────┘                                         │
│        │                                                  │
│        └── 定时检查所有6个任务的心跳                        │
└───────────────────────────────────────────────────────────┘
              │
    ┌─────────┴─────────┐
    │                   │
UART1 (有线)      HC05 (蓝牙)
    │                   │
    ▼                   ▼
Linux/PC          Linux/PC/Android
```

详细见[系统架构图](系统架构图.png)

## 任务列表

| 任务名 | 优先级 | 栈大小 | 功能 |
|--------|--------|--------|------|
| sensorTask | Normal | 1024B | 采集所有传感器数据 |
| lcdTask | BelowNormal | 1024B | LCD 屏幕显示 |
| uartTask | Normal | 1024B | 数据上传到 PC |
| cmdTask | High | 1024B | 处理外部命令 |
| fanTask | Normal | 1024B | 温度触发风扇控制 |
| monitorTask | Idle | 512B | 系统监控 + 喂狗 |

## 设计要点

### 滑动平均滤波器
- 传感器原始数据经过 8 点滑动平均滤波，抑制 BME280 测量噪声
- 环形缓冲区实现 O(1) 复杂度更新
- 滤波后数据用于 LCD 显示和风扇控制

### 硬件看门狗 + 任务健康监控
- IWDG 独立看门狗，4s 超时自动复位
- Monitor Task 以 2s 周期喂狗并检查所有 6 个任务的心跳
- 临界任务连续 3 次超时触发系统停机
- `GET /status` 返回每个任务存活时间和超时次数

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
- `GET /diag` 触发全系统自检
- 覆盖项：I2C、ADC、LCD、UART、电机、FSMC、BME280、Flash
- 返回结构化诊断报告

### 自动版本信息
- 编译时生成版本字符串（含日期时间）
- `GET /version` 返回固件标识

## API 接口协议

通过 UART1 或 UART2 (HC05) 发送命令控制。

### 支持的命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `GET /sensor` | 触发传感器数据上传 | `GET /sensor\r\n` |
| `GET /status` | 获取系统状态 + 任务健康 | `GET /status\r\n` |
| `GET /version` | 获取固件版本 | `GET /version\r\n` |
| `GET /diag` | 运行硬件自检诊断 | `GET /diag\r\n` |
| `SET /motor?speed=X` | 设置电机速度 (-100~100) | `SET /motor?speed=50\r\n` |
| `SET /fan?mode=auto` | 设置风扇自动模式 | `SET /fan?mode=auto\r\n` |
| `SET /fan?mode=manual` | 设置风扇手动模式 | `SET /fan?mode=manual\r\n` |

### Linux 调用示例

```bash
SERIAL_PORT=/dev/ttyUSB0

echo "GET /version" > $SERIAL_PORT
cat $SERIAL_PORT

echo "GET /status" > $SERIAL_PORT
cat $SERIAL_PORT

echo "GET /diag" > $SERIAL_PORT
cat $SERIAL_PORT

echo "SET /motor?speed=75" > $SERIAL_PORT

echo "SET /fan?mode=auto" > $SERIAL_PORT
echo "SET /fan?mode=manual" > $SERIAL_PORT
```

### 自检响应示例

```
=== DIAG REPORT [356 ms] ===
  [PASS] I2C_BME280: id=0x60
  [PASS] ADC: conversion ok
  [PASS] LCD: color test done
  [PASS] UART: tx ok
  [PASS] MOTOR: pwm ok
  [PASS] FSMC: rw ok
  [PASS] SENSOR: data ok
  [PASS] FLASH: param area ok
=== END ===
```

## 风扇状态机

| 状态 | 转速 | 升迁条件 | 降级条件 |
|------|------|---------|---------|
| IDLE | 0% | 温度 > 28°C → LOW | - |
| LOW | 25% | 温度 > 32°C → MEDIUM | 温度 < 26°C 且 >= 10s → IDLE |
| MEDIUM | 50% | 温度 > 36°C → HIGH | 温度 < 30°C 且 >= 10s → LOW |
| HIGH | 75% | 温度 > 40°C → MAX | 温度 < 34°C 且 >= 10s → MEDIUM |
| MAX | 100% | - | 温度 < 38°C 且 >= 10s → HIGH |

## 数据格式

### 传感器数据 (JSON)

```json
{"temp":25.50,"humid":60.2,"press":1013.2,"light":2048,"pm25":512}
```

### 系统状态

```
STATUS: fan_mode=0 fan_speed=50 uptime=12345
[OK] sensor: alive=500 misses=0
[OK] lcd: alive=499 misses=0
[OK] uart: alive=501 misses=0
[OK] cmd: alive=498 misses=0
[OK] fan: alive=500 misses=0
[OK] monitor: alive=2000 misses=0
```

### 版本信息

```
VERSION: STM32-ENR v1.2.0 (May  3 2026 14:30:00)
```

## 项目结构

```
Core/
├── Inc/
│   ├── main.h              # 主头文件
│   ├── FreeRTOSConfig.h    # FreeRTOS 配置 (heap=15KB)
│   ├── sensor_data.h       # 传感器数据结构
│   ├── bme280.h            # BME280 驱动
│   ├── lcd.h               # LCD 驱动
│   ├── motor.h             # 电机驱动
│   ├── filter.h            # 数字滤波器
│   ├── watchdog.h          # 看门狗 + 任务监控
│   ├── storage.h           # Flash 参数存储
│   ├── version.h           # 编译版本信息
│   ├── diagnostic.h        # 硬件自检框架
│   └── usart.h             # 串口驱动
└── Src/
    ├── main.c              # 主程序入口
    ├── freertos.c          # FreeRTOS 任务配置
    ├── bme280.c            # BME280 驱动实现
    ├── lcd.c               # LCD 驱动 (含完整字模)
    ├── motor.c             # 电机驱动
    ├── filter.c            # 滤波器实现
    ├── watchdog.c          # 看门狗实现
    ├── storage.c           # Flash 存储实现
    ├── diagnostic.c        # 自检实现
    ├── usart.c             # UART DMA 空闲中断
    └── stm32f1xx_it.c      # 中断处理
```

## 编译

使用 Keil MDK (v5) ，编译并烧录到目标板。


## 注意事项

1. HC05 蓝牙模块默认波特率 115200
2. UART1 和 UART2 均使用 115200 波特率
3. 电机/风扇使用 TIM2 的 CH1 和 CH2 通道
4. BME280 使用 I2C1 接口 (PB6/PB7)
5. 光敏和 PM2.5 使用 ADC1 通道（根据实际硬件配置）
6. 系统参数存储在 Flash 地址 `0x0807F800`，占用 1 页
7. 首次上电自动初始化默认参数并写入Flash
8. **野火指南者兼容**: STBY引脚使用PA5，避免与SPI1_NSS(PA4)冲突
