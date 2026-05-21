#pragma once

#include "mavlink_types.h"
#include "checksum.h"

#ifndef MAVLINK_GET_CHANNEL_STATUS
static inline mavlink_status_t *mavlink_get_channel_status(uint8_t chan)
{
    static mavlink_status_t m_mavlink_status[MAVLINK_COMM_NUM_BUFFERS];
    return &m_mavlink_status[chan];
}
#endif

#ifndef MAVLINK_GET_CHANNEL_BUFFER
static inline mavlink_message_t *mavlink_get_channel_buffer(uint8_t chan)
{
    static mavlink_message_t m_mavlink_buffer[MAVLINK_COMM_NUM_BUFFERS];
    return &m_mavlink_buffer[chan];
}
#endif

static inline void mavlink_reset_channel_status(uint8_t chan)
{
    mavlink_status_t *status = mavlink_get_channel_status(chan);
    status->parse_state = MAVLINK_PARSE_STATE_IDLE;
}

static inline uint8_t _mav_trim_payload(const char *payload, uint8_t length)
{
    while (length > 1 && payload[length - 1] == 0) {
        length--;
    }
    return length;
}

static inline uint16_t mavlink_finalize_message_buffer(mavlink_message_t *msg,
    uint8_t system_id, uint8_t component_id,
    mavlink_status_t *status,
    uint8_t min_length, uint8_t length, uint8_t crc_extra)
{
    bool mavlink1 = (status->flags & 2) != 0;
    uint8_t signature_len = 0;
    uint8_t header_len = MAVLINK_CORE_HEADER_LEN + 1;

    if (mavlink1) {
        msg->magic = MAVLINK_STX_MAVLINK1;
        header_len = MAVLINK_CORE_HEADER_MAVLINK1_LEN + 1;
    } else {
        msg->magic = MAVLINK_STX;
    }

    msg->len = mavlink1 ? min_length : _mav_trim_payload(_MAV_PAYLOAD(msg), length);
    msg->sysid = system_id;
    msg->compid = component_id;
    msg->incompat_flags = 0;
    msg->compat_flags = 0;
    msg->seq = status->current_tx_seq;
    status->current_tx_seq++;

    uint8_t buf[MAVLINK_CORE_HEADER_LEN + 1];
    buf[0] = msg->magic;
    buf[1] = msg->len;
    if (mavlink1) {
        buf[2] = msg->seq;
        buf[3] = msg->sysid;
        buf[4] = msg->compid;
        buf[5] = msg->msgid & 0xFF;
    } else {
        buf[2] = msg->incompat_flags;
        buf[3] = msg->compat_flags;
        buf[4] = msg->seq;
        buf[5] = msg->sysid;
        buf[6] = msg->compid;
        buf[7] = msg->msgid & 0xFF;
        buf[8] = (msg->msgid >> 8) & 0xFF;
        buf[9] = (msg->msgid >> 16) & 0xFF;
    }

    uint16_t checksum = crc_calculate(&buf[1], header_len - 1);
    crc_accumulate_buffer(&checksum, _MAV_PAYLOAD(msg), msg->len);
    crc_accumulate(crc_extra, &checksum);

    mavlink_ck_a(msg) = (uint8_t)(checksum & 0xFF);
    mavlink_ck_b(msg) = (uint8_t)(checksum >> 8);
    msg->checksum = checksum;

    return msg->len + header_len + 2 + signature_len;
}

static inline uint16_t mavlink_finalize_message(mavlink_message_t *msg,
    uint8_t system_id, uint8_t component_id,
    uint8_t min_length, uint8_t length, uint8_t crc_extra)
{
    mavlink_status_t *status = mavlink_get_channel_status(MAVLINK_COMM_0);
    return mavlink_finalize_message_buffer(msg, system_id, component_id,
        status, min_length, length, crc_extra);
}

static inline uint16_t mavlink_msg_to_send_buffer(uint8_t *buf, const mavlink_message_t *msg)
{
    uint8_t signature_len = 0;
    uint8_t header_len;
    uint8_t length = msg->len;

    if (msg->magic == MAVLINK_STX_MAVLINK1) {
        header_len = MAVLINK_CORE_HEADER_MAVLINK1_LEN;
        buf[0] = msg->magic;
        buf[1] = length;
        buf[2] = msg->seq;
        buf[3] = msg->sysid;
        buf[4] = msg->compid;
        buf[5] = msg->msgid & 0xFF;
        memcpy(&buf[6], _MAV_PAYLOAD(msg), msg->len);
    } else {
        length = _mav_trim_payload(_MAV_PAYLOAD(msg), length);
        header_len = MAVLINK_CORE_HEADER_LEN;
        buf[0] = msg->magic;
        buf[1] = length;
        buf[2] = msg->incompat_flags;
        buf[3] = msg->compat_flags;
        buf[4] = msg->seq;
        buf[5] = msg->sysid;
        buf[6] = msg->compid;
        buf[7] = msg->msgid & 0xFF;
        buf[8] = (msg->msgid >> 8) & 0xFF;
        buf[9] = (msg->msgid >> 16) & 0xFF;
        memcpy(&buf[10], _MAV_PAYLOAD(msg), length);
    }

    uint8_t *ck = buf + header_len + 1 + length;
    ck[0] = mavlink_ck_a(msg);
    ck[1] = mavlink_ck_b(msg);
    return header_len + 1 + length + 2 + signature_len;
}

typedef enum {
    MAVLINK_FRAME_RESULT_NONE = 0,
    MAVLINK_FRAME_RESULT_OK = 1,
    MAVLINK_FRAME_RESULT_CRC_ERROR = 2
} mavlink_frame_result_t;

/* CRC extra lookup table for known messages */
static inline uint8_t mavlink_get_crc_extra(uint32_t msgid)
{
    switch (msgid) {
        case 0:     return 50;   /* HEARTBEAT */
        case 50000: return 187;  /* SENSOR_DATA */
        case 50001: return 92;   /* CMD_MOTOR */
        case 50002: return 113;  /* CMD_FAN */
        case 50003: return 45;   /* STATUS_REPORT */
        case 50004: return 178;  /* DIAG_REPORT */
        case 50005: return 201;  /* CMD_GET_DATA */
        case 50006: return 67;   /* CMD_GET_STATUS */
        case 50007: return 134;  /* CMD_GET_VERSION */
        case 50008: return 29;   /* VERSION_REPORT */
        case 50010: return 211;  /* OTA_BEGIN */
        case 50011: return 66;   /* OTA_DATA */
        case 50012: return 143;  /* OTA_ACK */
        case 50013: return 89;   /* OTA_COMPLETE */
        default:    return 0;
    }
}

static inline mavlink_frame_result_t mavlink_frame_char(uint8_t chan, uint8_t c,
    mavlink_message_t *r_message, mavlink_status_t *r_mavlink_status)
{
    mavlink_status_t *status = mavlink_get_channel_status(chan);
    mavlink_message_t *msg = mavlink_get_channel_buffer(chan);
    mavlink_frame_result_t result = MAVLINK_FRAME_RESULT_NONE;

    switch (status->parse_state) {
    case MAVLINK_PARSE_STATE_IDLE:
        if (c == MAVLINK_STX || c == MAVLINK_STX_MAVLINK1) {
            status->parse_state = MAVLINK_PARSE_STATE_GOT_STX;
            status->packet_idx = 0;
            memset(msg, 0, sizeof(mavlink_message_t));
            msg->magic = c;
        }
        break;

    case MAVLINK_PARSE_STATE_GOT_STX:
        if (c > MAVLINK_MAX_PAYLOAD_LEN) {
            status->parse_state = MAVLINK_PARSE_STATE_IDLE;
            break;
        }
        msg->len = c;
        status->parse_state = MAVLINK_PARSE_STATE_GOT_LENGTH;
        break;

    case MAVLINK_PARSE_STATE_GOT_LENGTH:
        if (msg->magic != MAVLINK_STX_MAVLINK1) {
            msg->incompat_flags = c;
            status->parse_state = MAVLINK_PARSE_STATE_GOT_INCOMPAT_FLAGS;
        } else {
            msg->seq = c;
            status->parse_state = MAVLINK_PARSE_STATE_GOT_SEQ;
        }
        break;

    case MAVLINK_PARSE_STATE_GOT_INCOMPAT_FLAGS:
        msg->compat_flags = c;
        status->parse_state = MAVLINK_PARSE_STATE_GOT_COMPAT_FLAGS;
        break;

    case MAVLINK_PARSE_STATE_GOT_COMPAT_FLAGS:
        msg->seq = c;
        status->parse_state = MAVLINK_PARSE_STATE_GOT_SEQ;
        break;

    case MAVLINK_PARSE_STATE_GOT_SEQ:
        msg->sysid = c;
        status->parse_state = MAVLINK_PARSE_STATE_GOT_SYSID;
        break;

    case MAVLINK_PARSE_STATE_GOT_SYSID:
        msg->compid = c;
        status->parse_state = MAVLINK_PARSE_STATE_GOT_COMPID;
        break;

    case MAVLINK_PARSE_STATE_GOT_COMPID:
        msg->msgid = c;
        if (msg->magic == MAVLINK_STX_MAVLINK1) {
            status->parse_state = MAVLINK_PARSE_STATE_GOT_PAYLOAD;
            status->packet_idx = 0;
        } else {
            status->parse_state = MAVLINK_PARSE_STATE_GOT_MSGID1;
        }
        break;

    case MAVLINK_PARSE_STATE_GOT_MSGID1:
        msg->msgid |= ((uint32_t)c << 8);
        status->parse_state = MAVLINK_PARSE_STATE_GOT_MSGID2;
        break;

    case MAVLINK_PARSE_STATE_GOT_MSGID2:
        msg->msgid |= ((uint32_t)c << 16);
        status->parse_state = MAVLINK_PARSE_STATE_GOT_PAYLOAD;
        status->packet_idx = 0;
        break;

    case MAVLINK_PARSE_STATE_GOT_PAYLOAD:
        if (status->packet_idx < msg->len) {
            ((uint8_t *)_MAV_PAYLOAD_NON_CONST(msg))[status->packet_idx] = c;
            status->packet_idx++;
        }
        if (status->packet_idx >= msg->len) {
            status->parse_state = MAVLINK_PARSE_STATE_GOT_CRC1;
        }
        break;

    case MAVLINK_PARSE_STATE_GOT_CRC1:
        msg->ck[0] = c;
        status->parse_state = MAVLINK_PARSE_STATE_GOT_CRC2;
        break;

    case MAVLINK_PARSE_STATE_GOT_CRC2:
    {
        msg->ck[1] = c;

        uint8_t header_len = (msg->magic == MAVLINK_STX_MAVLINK1) ?
            MAVLINK_CORE_HEADER_MAVLINK1_LEN : MAVLINK_CORE_HEADER_LEN;
        uint8_t hbuf[MAVLINK_CORE_HEADER_LEN + 1];

        hbuf[0] = msg->magic;
        hbuf[1] = msg->len;
        if (msg->magic == MAVLINK_STX_MAVLINK1) {
            hbuf[2] = msg->seq;
            hbuf[3] = msg->sysid;
            hbuf[4] = msg->compid;
            hbuf[5] = msg->msgid & 0xFF;
        } else {
            hbuf[2] = msg->incompat_flags;
            hbuf[3] = msg->compat_flags;
            hbuf[4] = msg->seq;
            hbuf[5] = msg->sysid;
            hbuf[6] = msg->compid;
            hbuf[7] = msg->msgid & 0xFF;
            hbuf[8] = (msg->msgid >> 8) & 0xFF;
            hbuf[9] = (msg->msgid >> 16) & 0xFF;
        }

        uint16_t computed_crc = crc_calculate(&hbuf[1], header_len);
        crc_accumulate_buffer(&computed_crc, _MAV_PAYLOAD(msg), msg->len);
        crc_accumulate(mavlink_get_crc_extra(msg->msgid), &computed_crc);

        uint16_t received_crc = (uint16_t)msg->ck[0] | ((uint16_t)msg->ck[1] << 8);

        status->parse_state = MAVLINK_PARSE_STATE_IDLE;

        if (computed_crc == received_crc) {
            memcpy(r_message, msg, sizeof(mavlink_message_t));
            status->msg_received++;
            status->packet_rx_success_count++;
            status->current_rx_seq = msg->seq;
            result = MAVLINK_FRAME_RESULT_OK;
        } else {
            status->parse_error++;
            status->packet_rx_drop_count++;
            result = MAVLINK_FRAME_RESULT_CRC_ERROR;
        }
        break;
    }

    default:
        status->parse_state = MAVLINK_PARSE_STATE_IDLE;
        break;
    }

    if (r_mavlink_status) {
        memcpy(r_mavlink_status, status, sizeof(mavlink_status_t));
    }
    return result;
}
