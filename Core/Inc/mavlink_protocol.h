#pragma once

#include "mavlink/mavlink_types.h"
#include "mavlink/mavlink_helpers.h"
#include "mavlink/protocol.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 系统标识 */
#define MAVLINK_SYS_ID      1
#define MAVLINK_COMP_ID     1

/* 回调函数类型 - 收到命令时触发 */
typedef void (*mavlink_cmd_callback_t)(const mavlink_message_t *msg);

/* 初始化 MAVLink 协议层 */
void MAVLink_Init(void);

/* 发送心跳包 */
void MAVLink_SendHeartbeat(UART_HandleTypeDef *huart);

/* 发送传感器数据 (已滤波) */
void MAVLink_SendSensorData(UART_HandleTypeDef *huart,
    float temperature, float humidity, float pressure,
    float light, float pm25, uint32_t timestamp,
    bool bme280_valid, bool light_valid, bool pm25_valid);

/* 发送系统状态报告 */
void MAVLink_SendStatusReport(UART_HandleTypeDef *huart,
    uint32_t uptime, int32_t fan_speed, uint8_t fan_mode,
    uint8_t task_count, uint8_t task_alive_mask, uint8_t wdt_resets);

/* 发送固件版本 */
void MAVLink_SendVersionReport(UART_HandleTypeDef *huart, const char *version);

/* 发送诊断报告 */
void MAVLink_SendDiagReport(UART_HandleTypeDef *huart, const uint8_t *diag_data);

/* 发送 OTA ACK */
void MAVLink_SendOTAACK(UART_HandleTypeDef *huart, uint8_t status, uint32_t written_size);

/* 发送 ACK 响应 (用 STATUS_REPORT 作为确认) */
void MAVLink_SendAck(UART_HandleTypeDef *huart, uint32_t ack_code);

/* 处理接收到的字节 (在 UART 中断/DMA 回调中调用) */
void MAVLink_ProcessByte(uint8_t c);

/* 注册命令回调 */
void MAVLink_RegisterCmdCallback(mavlink_cmd_callback_t cb);

/* 获取最近一次解析的消息 (用于外部处理) */
const mavlink_message_t *MAVLink_GetLastMessage(void);

#ifdef __cplusplus
}
#endif
