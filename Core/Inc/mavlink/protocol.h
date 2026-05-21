#pragma once

#include "mavlink_types.h"
#include "mavlink_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Custom message IDs (in the vendor-specific range: 50000+)
 */
#define MAVLINK_MSG_ID_SENSOR_DATA     50000
#define MAVLINK_MSG_ID_CMD_MOTOR       50001
#define MAVLINK_MSG_ID_CMD_FAN         50002
#define MAVLINK_MSG_ID_STATUS_REPORT   50003
#define MAVLINK_MSG_ID_DIAG_REPORT     50004
#define MAVLINK_MSG_ID_CMD_GET_DATA    50005
#define MAVLINK_MSG_ID_CMD_GET_STATUS  50006
#define MAVLINK_MSG_ID_CMD_GET_VERSION 50007
#define MAVLINK_MSG_ID_VERSION_REPORT  50008

/* ==================================================================== */
/*  HEARTBEAT (standard MAVLink message, id=0)                          */
/* ==================================================================== */
#define MAVLINK_MSG_ID_HEARTBEAT_LEN 9
#define MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN 9
#define MAVLINK_MSG_ID_HEARTBEAT_CRC 50

#define MAV_TYPE_GENERIC      0
#define MAV_AUTOPILOT_GENERIC 0
#define MAV_STATE_STANDBY     3
#define MAV_STATE_ACTIVE      4

typedef struct __mavlink_heartbeat_t {
    uint8_t type;
    uint8_t autopilot;
    uint8_t base_mode;
    uint32_t custom_mode;
    uint8_t system_status;
} mavlink_heartbeat_t;

static inline uint16_t mavlink_msg_heartbeat_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg,
    uint8_t type, uint8_t autopilot, uint8_t base_mode,
    uint32_t custom_mode, uint8_t system_status)
{
    uint8_t buf[MAVLINK_MSG_ID_HEARTBEAT_LEN];
    memset(buf, 0, sizeof(buf));
    buf[0] = custom_mode & 0xFF;
    buf[1] = (custom_mode >> 8) & 0xFF;
    buf[2] = (custom_mode >> 16) & 0xFF;
    buf[3] = (custom_mode >> 24) & 0xFF;
    buf[4] = type;
    buf[5] = autopilot;
    buf[6] = base_mode;
    buf[7] = system_status;
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_HEARTBEAT_LEN);
    msg->msgid = 0;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_HEARTBEAT_MIN_LEN,
        MAVLINK_MSG_ID_HEARTBEAT_LEN,
        MAVLINK_MSG_ID_HEARTBEAT_CRC);
}

static inline void mavlink_msg_heartbeat_decode(const mavlink_message_t *msg,
    mavlink_heartbeat_t *hb)
{
    const uint8_t *buf = (const uint8_t *)_MAV_PAYLOAD(msg);
    hb->custom_mode = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
                      ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    hb->type = buf[4];
    hb->autopilot = buf[5];
    hb->base_mode = buf[6];
    hb->system_status = buf[7];
}

/* ==================================================================== */
/*  SENSOR_DATA (50000) - Periodic sensor data broadcast                */
/* ==================================================================== */
#define MAVLINK_MSG_ID_SENSOR_DATA_LEN 25
#define MAVLINK_MSG_ID_SENSOR_DATA_MIN_LEN 25
#define MAVLINK_MSG_ID_SENSOR_DATA_CRC 187

#define SENSOR_VALID_BME280 0x01
#define SENSOR_VALID_LIGHT  0x02
#define SENSOR_VALID_PM25   0x04

typedef struct __mavlink_sensor_data_t {
    float temperature;
    float humidity;
    float pressure;
    float light;
    float pm25;
    uint32_t timestamp;
    uint8_t validity;
} mavlink_sensor_data_t;

static inline uint16_t mavlink_msg_sensor_data_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg,
    float temperature, float humidity, float pressure,
    float light, float pm25, uint32_t timestamp, uint8_t validity)
{
    uint8_t buf[MAVLINK_MSG_ID_SENSOR_DATA_LEN];
    memcpy(&buf[0], &temperature, 4);
    memcpy(&buf[4], &humidity, 4);
    memcpy(&buf[8], &pressure, 4);
    memcpy(&buf[12], &light, 4);
    memcpy(&buf[16], &pm25, 4);
    memcpy(&buf[20], &timestamp, 4);
    buf[24] = validity;
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_SENSOR_DATA_LEN);
    msg->msgid = MAVLINK_MSG_ID_SENSOR_DATA;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_SENSOR_DATA_MIN_LEN,
        MAVLINK_MSG_ID_SENSOR_DATA_LEN,
        MAVLINK_MSG_ID_SENSOR_DATA_CRC);
}

static inline void mavlink_msg_sensor_data_decode(const mavlink_message_t *msg,
    mavlink_sensor_data_t *data)
{
    const uint8_t *buf = (const uint8_t *)_MAV_PAYLOAD(msg);
    memcpy(&data->temperature, &buf[0], 4);
    memcpy(&data->humidity, &buf[4], 4);
    memcpy(&data->pressure, &buf[8], 4);
    memcpy(&data->light, &buf[12], 4);
    memcpy(&data->pm25, &buf[16], 4);
    memcpy(&data->timestamp, &buf[20], 4);
    data->validity = buf[24];
}

/* ==================================================================== */
/*  CMD_MOTOR (50001) - Set motor speed                                 */
/* ==================================================================== */
#define MAVLINK_MSG_ID_CMD_MOTOR_LEN 4
#define MAVLINK_MSG_ID_CMD_MOTOR_MIN_LEN 4
#define MAVLINK_MSG_ID_CMD_MOTOR_CRC 92

typedef struct __mavlink_cmd_motor_t {
    int32_t speed;
} mavlink_cmd_motor_t;

static inline uint16_t mavlink_msg_cmd_motor_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg, int32_t speed)
{
    uint8_t buf[4];
    memcpy(buf, &speed, 4);
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 4);
    msg->msgid = MAVLINK_MSG_ID_CMD_MOTOR;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_CMD_MOTOR_MIN_LEN,
        MAVLINK_MSG_ID_CMD_MOTOR_LEN,
        MAVLINK_MSG_ID_CMD_MOTOR_CRC);
}

static inline void mavlink_msg_cmd_motor_decode(const mavlink_message_t *msg,
    mavlink_cmd_motor_t *cmd)
{
    memcpy(&cmd->speed, _MAV_PAYLOAD(msg), 4);
}

/* ==================================================================== */
/*  CMD_FAN (50002) - Set fan mode (0=auto, 1=manual)                  */
/* ==================================================================== */
#define MAVLINK_MSG_ID_CMD_FAN_LEN 1
#define MAVLINK_MSG_ID_CMD_FAN_MIN_LEN 1
#define MAVLINK_MSG_ID_CMD_FAN_CRC 113

typedef struct __mavlink_cmd_fan_t {
    uint8_t mode;
} mavlink_cmd_fan_t;

static inline uint16_t mavlink_msg_cmd_fan_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg, uint8_t mode)
{
    uint8_t buf[1] = { mode };
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 1);
    msg->msgid = MAVLINK_MSG_ID_CMD_FAN;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_CMD_FAN_MIN_LEN,
        MAVLINK_MSG_ID_CMD_FAN_LEN,
        MAVLINK_MSG_ID_CMD_FAN_CRC);
}

static inline void mavlink_msg_cmd_fan_decode(const mavlink_message_t *msg,
    mavlink_cmd_fan_t *cmd)
{
    cmd->mode = ((const uint8_t *)_MAV_PAYLOAD(msg))[0];
}

/* ==================================================================== */
/*  STATUS_REPORT (50003) - System status response                      */
/* ==================================================================== */
#define MAVLINK_MSG_ID_STATUS_REPORT_LEN 16
#define MAVLINK_MSG_ID_STATUS_REPORT_MIN_LEN 16
#define MAVLINK_MSG_ID_STATUS_REPORT_CRC 45

typedef struct __mavlink_status_report_t {
    uint32_t uptime;
    int32_t fan_speed;
    uint8_t fan_mode;
    uint8_t task_count;
    uint8_t task_alive_mask;
    uint8_t wdt_resets;
} mavlink_status_report_t;

static inline uint16_t mavlink_msg_status_report_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg,
    uint32_t uptime, int32_t fan_speed, uint8_t fan_mode,
    uint8_t task_count, uint8_t task_alive_mask, uint8_t wdt_resets)
{
    uint8_t buf[MAVLINK_MSG_ID_STATUS_REPORT_LEN];
    memset(buf, 0, sizeof(buf));
    memcpy(&buf[0], &uptime, 4);
    memcpy(&buf[4], &fan_speed, 4);
    buf[8] = fan_mode;
    buf[9] = task_count;
    buf[10] = task_alive_mask;
    buf[11] = wdt_resets;
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, MAVLINK_MSG_ID_STATUS_REPORT_LEN);
    msg->msgid = MAVLINK_MSG_ID_STATUS_REPORT;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_STATUS_REPORT_MIN_LEN,
        MAVLINK_MSG_ID_STATUS_REPORT_LEN,
        MAVLINK_MSG_ID_STATUS_REPORT_CRC);
}

static inline void mavlink_msg_status_report_decode(const mavlink_message_t *msg,
    mavlink_status_report_t *report)
{
    const uint8_t *buf = (const uint8_t *)_MAV_PAYLOAD(msg);
    memcpy(&report->uptime, &buf[0], 4);
    memcpy(&report->fan_speed, &buf[4], 4);
    report->fan_mode = buf[8];
    report->task_count = buf[9];
    report->task_alive_mask = buf[10];
    report->wdt_resets = buf[11];
}

/* ==================================================================== */
/*  DIAG_REPORT (50004) - Full diagnostic report                        */
/* ==================================================================== */
#define MAVLINK_MSG_ID_DIAG_REPORT_LEN 32
#define MAVLINK_MSG_ID_DIAG_REPORT_MIN_LEN 32
#define MAVLINK_MSG_ID_DIAG_REPORT_CRC 178

typedef struct __mavlink_diag_report_t {
    uint8_t data[32];
} mavlink_diag_report_t;

static inline uint16_t mavlink_msg_diag_report_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg, const uint8_t *diag_data)
{
    memset(_MAV_PAYLOAD_NON_CONST(msg), 0, MAVLINK_MSG_ID_DIAG_REPORT_LEN);
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), diag_data, MAVLINK_MSG_ID_DIAG_REPORT_LEN);
    msg->msgid = MAVLINK_MSG_ID_DIAG_REPORT;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_DIAG_REPORT_MIN_LEN,
        MAVLINK_MSG_ID_DIAG_REPORT_LEN,
        MAVLINK_MSG_ID_DIAG_REPORT_CRC);
}

static inline void mavlink_msg_diag_report_decode(const mavlink_message_t *msg,
    uint8_t *diag_data)
{
    memcpy(diag_data, _MAV_PAYLOAD(msg), MAVLINK_MSG_ID_DIAG_REPORT_LEN);
}

/* ==================================================================== */
/*  CMD_GET_DATA (50005) - Request sensor data                          */
/* ==================================================================== */
#define MAVLINK_MSG_ID_CMD_GET_DATA_LEN 0
#define MAVLINK_MSG_ID_CMD_GET_DATA_MIN_LEN 0
#define MAVLINK_MSG_ID_CMD_GET_DATA_CRC 201

/* Payload is empty - just the message ID triggers data send */
static inline uint16_t mavlink_msg_cmd_get_data_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg)
{
    memset(_MAV_PAYLOAD_NON_CONST(msg), 0, 0);
    msg->msgid = MAVLINK_MSG_ID_CMD_GET_DATA;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_CMD_GET_DATA_MIN_LEN,
        MAVLINK_MSG_ID_CMD_GET_DATA_LEN,
        MAVLINK_MSG_ID_CMD_GET_DATA_CRC);
}

/* ==================================================================== */
/*  CMD_GET_STATUS (50006) - Request system status                      */
/* ==================================================================== */
#define MAVLINK_MSG_ID_CMD_GET_STATUS_LEN 0
#define MAVLINK_MSG_ID_CMD_GET_STATUS_MIN_LEN 0
#define MAVLINK_MSG_ID_CMD_GET_STATUS_CRC 67

static inline uint16_t mavlink_msg_cmd_get_status_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg)
{
    memset(_MAV_PAYLOAD_NON_CONST(msg), 0, 0);
    msg->msgid = MAVLINK_MSG_ID_CMD_GET_STATUS;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_CMD_GET_STATUS_MIN_LEN,
        MAVLINK_MSG_ID_CMD_GET_STATUS_LEN,
        MAVLINK_MSG_ID_CMD_GET_STATUS_CRC);
}

/* ==================================================================== */
/*  CMD_GET_VERSION (50007) - Request firmware version                  */
/* ==================================================================== */
#define MAVLINK_MSG_ID_CMD_GET_VERSION_LEN 0
#define MAVLINK_MSG_ID_CMD_GET_VERSION_MIN_LEN 0
#define MAVLINK_MSG_ID_CMD_GET_VERSION_CRC 134

static inline uint16_t mavlink_msg_cmd_get_version_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg)
{
    memset(_MAV_PAYLOAD_NON_CONST(msg), 0, 0);
    msg->msgid = MAVLINK_MSG_ID_CMD_GET_VERSION;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_CMD_GET_VERSION_MIN_LEN,
        MAVLINK_MSG_ID_CMD_GET_VERSION_LEN,
        MAVLINK_MSG_ID_CMD_GET_VERSION_CRC);
}

/* ==================================================================== */
/*  VERSION_REPORT (50008) - Version string response                    */
/* ==================================================================== */
#define MAVLINK_MSG_ID_VERSION_REPORT_LEN 32
#define MAVLINK_MSG_ID_VERSION_REPORT_MIN_LEN 32
#define MAVLINK_MSG_ID_VERSION_REPORT_CRC 29

typedef struct __mavlink_version_report_t {
    uint8_t version[32];
} mavlink_version_report_t;

static inline uint16_t mavlink_msg_version_report_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg, const char *version_str)
{
    memset(_MAV_PAYLOAD_NON_CONST(msg), 0, MAVLINK_MSG_ID_VERSION_REPORT_LEN);
    strncpy((char *)_MAV_PAYLOAD_NON_CONST(msg), version_str, 31);
    msg->msgid = MAVLINK_MSG_ID_VERSION_REPORT;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_VERSION_REPORT_MIN_LEN,
        MAVLINK_MSG_ID_VERSION_REPORT_LEN,
        MAVLINK_MSG_ID_VERSION_REPORT_CRC);
}

static inline void mavlink_msg_version_report_decode(const mavlink_message_t *msg,
    char *version_str, uint8_t max_len)
{
    uint8_t copy_len = msg->len < max_len ? msg->len : max_len - 1;
    memcpy(version_str, _MAV_PAYLOAD(msg), copy_len);
    version_str[copy_len] = '\0';
}

/* ==================================================================== */
/*  OTA_BEGIN (50010) - Start OTA firmware download                     */
/* ==================================================================== */
#define MAVLINK_MSG_ID_OTA_BEGIN         50010
#define MAVLINK_MSG_ID_OTA_BEGIN_LEN     8
#define MAVLINK_MSG_ID_OTA_BEGIN_MIN_LEN 8
#define MAVLINK_MSG_ID_OTA_BEGIN_CRC     211

typedef struct __mavlink_ota_begin_t {
    uint32_t total_size;
    uint32_t expected_crc;
} mavlink_ota_begin_t;

static inline uint16_t mavlink_msg_ota_begin_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg, uint32_t total_size, uint32_t expected_crc)
{
    uint8_t buf[8];
    memcpy(&buf[0], &total_size, 4);
    memcpy(&buf[4], &expected_crc, 4);
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 8);
    msg->msgid = MAVLINK_MSG_ID_OTA_BEGIN;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_OTA_BEGIN_MIN_LEN,
        MAVLINK_MSG_ID_OTA_BEGIN_LEN,
        MAVLINK_MSG_ID_OTA_BEGIN_CRC);
}

static inline void mavlink_msg_ota_begin_decode(const mavlink_message_t *msg,
    mavlink_ota_begin_t *cmd)
{
    const uint8_t *buf = (const uint8_t *)_MAV_PAYLOAD(msg);
    memcpy(&cmd->total_size, &buf[0], 4);
    memcpy(&cmd->expected_crc, &buf[4], 4);
}

/* ==================================================================== */
/*  OTA_DATA (50011) - Firmware chunk                                   */
/* ==================================================================== */
#define MAVLINK_MSG_ID_OTA_DATA          50011
#define MAVLINK_MSG_ID_OTA_DATA_LEN      132
#define MAVLINK_MSG_ID_OTA_DATA_MIN_LEN  132
#define MAVLINK_MSG_ID_OTA_DATA_CRC      66

#define OTA_DATA_CHUNK_SIZE              128

typedef struct __mavlink_ota_data_t {
    uint32_t offset;
    uint8_t data[OTA_DATA_CHUNK_SIZE];
} mavlink_ota_data_t;

static inline uint16_t mavlink_msg_ota_data_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg, uint32_t offset, const uint8_t *data)
{
    uint8_t buf[4 + OTA_DATA_CHUNK_SIZE];
    memset(buf, 0, sizeof(buf));
    memcpy(&buf[0], &offset, 4);
    memcpy(&buf[4], data, OTA_DATA_CHUNK_SIZE);
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, sizeof(buf));
    msg->msgid = MAVLINK_MSG_ID_OTA_DATA;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_OTA_DATA_MIN_LEN,
        MAVLINK_MSG_ID_OTA_DATA_LEN,
        MAVLINK_MSG_ID_OTA_DATA_CRC);
}

static inline void mavlink_msg_ota_data_decode(const mavlink_message_t *msg,
    mavlink_ota_data_t *cmd)
{
    const uint8_t *buf = (const uint8_t *)_MAV_PAYLOAD(msg);
    memcpy(&cmd->offset, &buf[0], 4);
    uint16_t copy_len = msg->len > 4 ? msg->len - 4 : 0;
    if (copy_len > OTA_DATA_CHUNK_SIZE) copy_len = OTA_DATA_CHUNK_SIZE;
    memset(cmd->data, 0, OTA_DATA_CHUNK_SIZE);
    memcpy(cmd->data, &buf[4], copy_len);
}

/* ==================================================================== */
/*  OTA_ACK (50012) - Acknowledge for each chunk                        */
/* ==================================================================== */
#define MAVLINK_MSG_ID_OTA_ACK          50012
#define MAVLINK_MSG_ID_OTA_ACK_LEN      5
#define MAVLINK_MSG_ID_OTA_ACK_MIN_LEN  5
#define MAVLINK_MSG_ID_OTA_ACK_CRC      143

#define OTA_ACK_OK      0
#define OTA_ACK_ERROR   1
#define OTA_ACK_BUSY    2

typedef struct __mavlink_ota_ack_t {
    uint8_t status;
    uint32_t written_size;
} mavlink_ota_ack_t;

static inline uint16_t mavlink_msg_ota_ack_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg, uint8_t status, uint32_t written_size)
{
    uint8_t buf[5];
    buf[0] = status;
    memcpy(&buf[1], &written_size, 4);
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 5);
    msg->msgid = MAVLINK_MSG_ID_OTA_ACK;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_OTA_ACK_MIN_LEN,
        MAVLINK_MSG_ID_OTA_ACK_LEN,
        MAVLINK_MSG_ID_OTA_ACK_CRC);
}

static inline void mavlink_msg_ota_ack_decode(const mavlink_message_t *msg,
    mavlink_ota_ack_t *ack)
{
    const uint8_t *buf = (const uint8_t *)_MAV_PAYLOAD(msg);
    ack->status = buf[0];
    memcpy(&ack->written_size, &buf[1], 4);
}

/* ==================================================================== */
/*  OTA_COMPLETE (50013) - Finalize OTA, trigger reboot                 */
/* ==================================================================== */
#define MAVLINK_MSG_ID_OTA_COMPLETE       50013
#define MAVLINK_MSG_ID_OTA_COMPLETE_LEN   4
#define MAVLINK_MSG_ID_OTA_COMPLETE_MIN_LEN 4
#define MAVLINK_MSG_ID_OTA_COMPLETE_CRC   89

typedef struct __mavlink_ota_complete_t {
    uint32_t crc32;
} mavlink_ota_complete_t;

static inline uint16_t mavlink_msg_ota_complete_pack(uint8_t sysid, uint8_t compid,
    mavlink_message_t *msg, uint32_t crc32)
{
    uint8_t buf[4];
    memcpy(buf, &crc32, 4);
    memcpy(_MAV_PAYLOAD_NON_CONST(msg), buf, 4);
    msg->msgid = MAVLINK_MSG_ID_OTA_COMPLETE;
    return mavlink_finalize_message(msg, sysid, compid,
        MAVLINK_MSG_ID_OTA_COMPLETE_MIN_LEN,
        MAVLINK_MSG_ID_OTA_COMPLETE_LEN,
        MAVLINK_MSG_ID_OTA_COMPLETE_CRC);
}

static inline void mavlink_msg_ota_complete_decode(const mavlink_message_t *msg,
    mavlink_ota_complete_t *cmd)
{
    memcpy(&cmd->crc32, _MAV_PAYLOAD(msg), 4);
}

/* ==================================================================== */
/*  Update CRC extra lookup table                                       */
/* ==================================================================== */
/* 已统一在 mavlink_helpers.h 中维护                                    */

#ifdef __cplusplus
}
#endif
