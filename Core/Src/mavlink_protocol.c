#include "mavlink_protocol.h"
#include "mavlink/checksum.h"
#include "mavlink/mavlink_helpers.h"
#include <string.h>

#define TX_BUF_SIZE 280

static uint8_t s_tx_buf[TX_BUF_SIZE];
static mavlink_message_t s_rx_msg;
static mavlink_cmd_callback_t s_cmd_callback = NULL;

void MAVLink_Init(void)
{
    mavlink_reset_channel_status(MAVLINK_COMM_0);
    s_cmd_callback = NULL;
}

void MAVLink_RegisterCmdCallback(mavlink_cmd_callback_t cb)
{
    s_cmd_callback = cb;
}

const mavlink_message_t *MAVLink_GetLastMessage(void)
{
    return &s_rx_msg;
}

static void send_buffer(UART_HandleTypeDef *huart, const uint8_t *buf, uint16_t len)
{
    if (huart && huart->Instance) {
        HAL_UART_Transmit(huart, (uint8_t *)buf, len, 100);
    }
}

void MAVLink_SendHeartbeat(UART_HandleTypeDef *huart)
{
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(MAVLINK_SYS_ID, MAVLINK_COMP_ID, &msg,
        MAV_TYPE_GENERIC, MAV_AUTOPILOT_GENERIC, 0, 0, MAV_STATE_ACTIVE);
    uint16_t len = mavlink_msg_to_send_buffer(s_tx_buf, &msg);
    send_buffer(huart, s_tx_buf, len);
}

void MAVLink_SendSensorData(UART_HandleTypeDef *huart,
    float temperature, float humidity, float pressure,
    float light, float pm25, uint32_t timestamp,
    bool bme280_valid, bool light_valid, bool pm25_valid)
{
    mavlink_message_t msg;
    uint8_t validity = 0;
    if (bme280_valid) validity |= SENSOR_VALID_BME280;
    if (light_valid)  validity |= SENSOR_VALID_LIGHT;
    if (pm25_valid)   validity |= SENSOR_VALID_PM25;

    mavlink_msg_sensor_data_pack(MAVLINK_SYS_ID, MAVLINK_COMP_ID, &msg,
        temperature, humidity, pressure, light, pm25, timestamp, validity);
    uint16_t len = mavlink_msg_to_send_buffer(s_tx_buf, &msg);
    send_buffer(huart, s_tx_buf, len);
}

void MAVLink_SendStatusReport(UART_HandleTypeDef *huart,
    uint32_t uptime, int32_t fan_speed, uint8_t fan_mode,
    uint8_t task_count, uint8_t task_alive_mask, uint8_t wdt_resets)
{
    mavlink_message_t msg;
    mavlink_msg_status_report_pack(MAVLINK_SYS_ID, MAVLINK_COMP_ID, &msg,
        uptime, fan_speed, fan_mode, task_count, task_alive_mask, wdt_resets);
    uint16_t len = mavlink_msg_to_send_buffer(s_tx_buf, &msg);
    send_buffer(huart, s_tx_buf, len);
}

void MAVLink_SendVersionReport(UART_HandleTypeDef *huart, const char *version)
{
    mavlink_message_t msg;
    mavlink_msg_version_report_pack(MAVLINK_SYS_ID, MAVLINK_COMP_ID, &msg, version);
    uint16_t len = mavlink_msg_to_send_buffer(s_tx_buf, &msg);
    send_buffer(huart, s_tx_buf, len);
}

void MAVLink_SendDiagReport(UART_HandleTypeDef *huart, const uint8_t *diag_data)
{
    mavlink_message_t msg;
    mavlink_msg_diag_report_pack(MAVLINK_SYS_ID, MAVLINK_COMP_ID, &msg, diag_data);
    uint16_t len = mavlink_msg_to_send_buffer(s_tx_buf, &msg);
    send_buffer(huart, s_tx_buf, len);
}

void MAVLink_SendAck(UART_HandleTypeDef *huart, uint32_t ack_code)
{
    mavlink_message_t msg;
    mavlink_msg_status_report_pack(MAVLINK_SYS_ID, MAVLINK_COMP_ID, &msg,
        ack_code, 0, 0, 0, 0, 0);
    uint16_t len = mavlink_msg_to_send_buffer(s_tx_buf, &msg);
    send_buffer(huart, s_tx_buf, len);
}

void MAVLink_SendOTAACK(UART_HandleTypeDef *huart, uint8_t status, uint32_t written_size)
{
    mavlink_message_t msg;
    mavlink_msg_ota_ack_pack(MAVLINK_SYS_ID, MAVLINK_COMP_ID, &msg,
        status, written_size);
    uint16_t len = mavlink_msg_to_send_buffer(s_tx_buf, &msg);
    send_buffer(huart, s_tx_buf, len);
}

void MAVLink_ProcessByte(uint8_t c)
{
    mavlink_status_t status;
    mavlink_frame_result_t result;

    result = mavlink_frame_char(MAVLINK_COMM_0, c, &s_rx_msg, &status);

    if (result == MAVLINK_FRAME_RESULT_OK) {
        if (s_cmd_callback) {
            s_cmd_callback(&s_rx_msg);
        }
    }
}
