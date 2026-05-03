# STM32F103VET6 传感器监控系统

基于 STM32F103VET6 核心板 + FreeRTOS 的多传感器数据采集与控制系统。

## 硬件配置

- **核心板**: STM32F103VET6 (ARM Cortex-M3)
- **操作系统**: FreeRTOS
- **通信接口**: UART1 (有线) / UART2 + HC05 (蓝牙)

## 外设列表

| 外设 | 接口 | 功能 |
|------|------|------|
| LCD 屏幕 | FSMC | 显示传感器数据 |
| HC05 蓝牙模块 | UART2 DMA 空闲中断 | 无线数据传输 |
| BME280 传感器 | I2C | 温度、湿度、气压采集 |
| 光敏电阻 | ADC | 环境光线强度采集 |
| PM2.5 传感器 | ADC | 颗粒物浓度采集 |
| TB6612 电机驱动 | TIM2 PWM | 直流电机/风扇控制 |

## 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                      FreeRTOS                           │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐   │
│  │ Sensor  │  │  LCD    │  │  UART   │  │  Fan    │   │
│  │  Task   │  │  Task   │  │  Task   │  │  Task   │   │
│  └────┬────┘  └────▲────┘  └────▲────┘  └────▲────┘   │
│       │            │            │            │          │
│       ▼            │            │            │          │
│  ┌─────────────────────────────────────────────┐       │
│  │              dataQueue (消息队列)             │       │
│  └─────────────────────────────────────────────┘       │
│                                                         │
│                        ┌───────┐                       │
│                        │ Cmd   │                       │
│                        │ Task  │                       │
│                        └───┬───┘                       │
│                            │                           │
│                    cmdQueue (命令队列)                  │
└─────────────────────────────────────────────────────────┘
                            │
              ┌─────────────┴─────────────┐
              │                           │
         UART1 (有线)              HC05 (蓝牙)
              │                           │
              ▼                           ▼
         Linux/PC                   Linux/PC
```

## 任务列表

| 任务名 | 优先级 | 栈大小 | 功能 |
|--------|--------|--------|------|
| sensorTask | Normal | 1024B | 采集所有传感器数据 |
| lcdTask | BelowNormal | 1024B | LCD 屏幕显示 |
| uartTask | Normal | 1024B | 数据上传到 PC |
| cmdTask | High | 1024B | 处理外部命令 |
| fanTask | Normal | 1024B | 温度触发风扇控制 |

## API 接口协议

通过 UART1 或 UART2 (HC05) 发送命令控制。

### 支持的命令

| 命令 | 说明 | 示例 |
|------|------|------|
| `GET /sensor` | 触发传感器数据上传 | `GET /sensor\r\n` |
| `GET /status` | 获取系统状态 | `GET /status\r\n` |
| `GET /version` | 获取固件版本 | `GET /version\r\n` |
| `SET /motor?speed=X` | 设置电机速度 (-100~100) | `SET /motor?speed=50\r\n` |
| `SET /fan?mode=auto` | 设置风扇为自动模式 | `SET /fan?mode=auto\r\n` |
| `SET /fan?mode=manual` | 设置风扇为手动模式 | `SET /fan?mode=manual\r\n` |

### Linux 端调用示例

```bash
# 读取串口设备 (根据实际情况选择 /dev/ttyUSB0 或 /dev/ttyS0)
SERIAL_PORT=/dev/ttyUSB0

# 获取固件版本
echo "GET /version" > $SERIAL_PORT
cat $SERIAL_PORT

# 获取系统状态
echo "GET /status" > $SERIAL_PORT
cat $SERIAL_PORT

# 设置电机速度
echo "SET /motor?speed=75" > $SERIAL_PORT

# 切换风扇模式
echo "SET /fan?mode=auto" > $SERIAL_PORT
echo "SET /fan?mode=manual" > $SERIAL_PORT
```

## 自动风扇控制

风扇任务根据温度自动调节转速：

| 温度范围 | 风扇速度 |
|---------|---------|
| > 35°C | 100% (全速) |
| 30-35°C | 50% (半速) |
| 25-30°C | 25% (低速) |
| < 25°C | 0% (关闭) |

## 数据格式

### 传感器数据 (JSON)

```json
{"temp":25.50,"humid":60.2,"press":1013.2,"light":2048,"pm25":512}
```

### 系统状态

```
STATUS: fan_mode=0 fan_speed=50 uptime=12345
```

### 版本信息

```
VERSION: STM32-F103VET6-FreeRTOS v1.0.0
```

## 项目结构

```
Core/
├── Inc/
│   ├── main.h
│   ├── FreeRTOS.h
│   ├── cmsis_os.h
│   ├── sensor_data.h      # 传感器数据结构
│   ├── bme280.h           # BME280 驱动
│   ├── lcd.h              # LCD 驱动
│   ├── motor.h            # 电机驱动
│   └── usart.h            # 串口驱动
└── Src/
    ├── main.c
    ├── freertos.c          # FreeRTOS 任务配置
    ├── bme280.c
    ├── lcd.c
    ├── motor.c
    ├── usart.c             # UART DMA 空闲中断
    └── stm32f1xx_it.c      # 中断处理
```

## 编译

使用 Keil MDK 或 STM32CubeIDE 打开项目，编译并烧录到目标板。

## 注意事项

1. HC05 蓝牙模块默认波特率为 115200
2. UART1 和 UART2 均使用 115200 波特率
3. 电机/风扇使用 TIM2 的 CH1 和 CH2 通道
4. BME280 使用 I2C1 接口 (PB6/PB7)
5. 光敏和 PM2.5 使用 ADC1 通道 (请根据实际硬件配置)


