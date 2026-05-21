# STM32F407VG 传感器监控系统

基于 STM32F407VG (Cortex-M4F, 168MHz) + FreeRTOS 的多传感器数据采集与控制系统。

## 外设列表

| 外设 | 接口 | 功能 |
|------|------|------|
| LCD (ILI9341) | SPI2 | 320×240 显示 |
| HC05 蓝牙 | UART2 DMA | 无线数据 + OTA |
| BME280 | I2C1 | 温度/湿度/气压 |
| 光敏电阻 | ADC1 CH10 | 环境光照 |
| PM2.5 | ADC1 CH12 | 颗粒物浓度 |
| TB6612 电机 | TIM2 PWM | 风扇控制 |

## 引脚分配

| 功能 | 引脚 | 功能 | 引脚 |
|------|------|------|------|
| I2C1_SCL/SDA | PB6/PB7 | USART1 TX/RX | PA9/PA10 |
| USART2 TX/RX | PA2/PA3 | ADC1 CH10/CH12 | PC0/PC2 |
| TIM2 CH3/CH4 | PB10/PB11 | AIN1/AIN2 | PA0/PA1 |
| BIN1/BIN2 | PB0/PB1 | STBY | PA5 |
| SPI2_SCK/MOSI | PB13/PB15 | LCD_CS/DC/RST | PD7/PD4/PD5 |

## 系统架构

```
生产者（纯采集，发消息到队列）
  BME280Task ─── I2C1 ──→ distributorTask ──┬── LcdTask (SPI2 LCD)
  LightTask  ─── ADC1 ──→ (滤波+写共享池)    ├── UartTask (MAVLink)
  PM25Task   ─── ADC1 ──→ (发EventFlags)     └── FanTask (只读温度)
                     ↑
               ADC1 由 osMutex 保护

  CmdTask     ─── MAVLink 命令处理 + OTA
  MonitorTask ─── IWDG 喂狗 + 9 任务健康检查
```

## 通信协议 — MAVLink v2

| ID | 方向 | 消息 | 说明 |
|----|------|------|------|
| 0 | 双向 | HEARTBEAT | 心跳 |
| 50000 | STM32→PC | SENSOR_DATA | 温度/湿度/气压/光照/PM2.5 |
| 50001 | PC→STM32 | CMD_MOTOR | 设电机速度 (自动切手动模式) |
| 50002 | PC→STM32 | CMD_FAN | 风扇模式 (0=自动, 1=手动) |
| 50003 | STM32→PC | STATUS_REPORT | 系统状态 |
| 50004 | STM32→PC | DIAG_REPORT | 自检报告 |
| 50005~07 | PC→STM32 | CMD_GET_DATA/STATUS/VERSION | 查询命令 |
| 50008 | STM32→PC | VERSION_REPORT | 固件版本 |
| 50010~13 | 双向 | OTA_BEGIN/DATA/ACK/COMPLETE | 固件升级 |

## OTA 升级

**Flash 布局：**
```
0x08000000  Bootloader (128KB)
0x08020000  App (384KB)
0x08080000  OTA 下载区 (384KB)
0x080E0000  Storage (Sector 7)
```

**首次烧录：** 先用 ST-Link 烧 Bootloader，再烧 App。之后所有升级走蓝牙：

```bash
fromelf --bin --output=firmware.bin MDK-ARM/stm32-enr/stm32-enr.axf
python Tools/ota_update.py COM3 firmware.bin
```

**升级流程：** PC 发 OTA_BEGIN → STM32 擦 OTA 区 → PC 逐包发 OTA_DATA (128B/包) → OTA_COMPLETE → STM32 校验 CRC → 写魔数 → 重启 → Bootloader 搬 OTA 区到 App 区 → 跳 App

## 设计要点

- **三层解耦**：生产者(纯采集) → 解析任务(滤波+写共享池) → 消费者(零拷贝读)，BME280 温湿压原子写入，风扇响应延迟 ~10ms
- **ADC1 互斥锁**：LightTask 和 PM25Task 共用 ADC1，osMutex 保证通道切换原子性
- **风扇状态机**：5 状态迟滞控制 (IDLE→LOW→MEDIUM→HIGH→MAX)，10s 防抖
- **看门狗**：IWDG 4s 超时，MonitorTask 检查 9 任务心跳，临界任务连续超时触发系统复位
- **手动控制覆盖**：CMD_MOTOR 自动切手动模式，FanTask 跳过温度控制；CMD_FAN mode=0 恢复自动
- **系统参数存储**：Flash Sector 7，CRC16 校验，上电自动恢复

## 注意事项

1. HC05 需先 AT 配置为 115200 波特率
2. 首次上电 BME280 自动初始化，系统参数写入 Flash
3. 编译后需用 `fromelf` 转 .bin 文件再 OTA
