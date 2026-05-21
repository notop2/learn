#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAVLINK_MAX_PAYLOAD_LEN 255
#define MAVLINK_CORE_HEADER_LEN 9
#define MAVLINK_CORE_HEADER_MAVLINK1_LEN 5
#define MAVLINK_NUM_HEADER_BYTES (MAVLINK_CORE_HEADER_LEN + 1)
#define MAVLINK_NUM_CHECKSUM_BYTES 2
#define MAVLINK_NUM_NON_PAYLOAD_BYTES (MAVLINK_NUM_HEADER_BYTES + MAVLINK_NUM_CHECKSUM_BYTES)
#define MAVLINK_SIGNATURE_BLOCK_LEN 13
#define MAVLINK_MAX_PACKET_LEN (MAVLINK_MAX_PAYLOAD_LEN + MAVLINK_NUM_NON_PAYLOAD_BYTES + MAVLINK_SIGNATURE_BLOCK_LEN)

#define MAVLINK_STX 253
#define MAVLINK_STX_MAVLINK1 0xFE

#define MAVLINK_IFLAG_SIGNED 0x01

#define _MAV_PAYLOAD(msg) ((const char *)(&((msg)->payload64[0])))
#define _MAV_PAYLOAD_NON_CONST(msg) ((char *)(&((msg)->payload64[0])))
#define mavlink_ck_a(msg) *((msg)->len + (uint8_t *)_MAV_PAYLOAD_NON_CONST(msg))
#define mavlink_ck_b(msg) *(((msg)->len + (uint16_t)1) + (uint8_t *)_MAV_PAYLOAD_NON_CONST(msg))

typedef enum {
    MAVLINK_COMM_0,
    MAVLINK_COMM_1,
    MAVLINK_COMM_2,
    MAVLINK_COMM_3
} mavlink_channel_t;

#ifndef MAVLINK_COMM_NUM_BUFFERS
#define MAVLINK_COMM_NUM_BUFFERS 4
#endif

typedef enum {
    MAVLINK_PARSE_STATE_UNINIT = 0,
    MAVLINK_PARSE_STATE_IDLE,
    MAVLINK_PARSE_STATE_GOT_STX,
    MAVLINK_PARSE_STATE_GOT_LENGTH,
    MAVLINK_PARSE_STATE_GOT_INCOMPAT_FLAGS,
    MAVLINK_PARSE_STATE_GOT_COMPAT_FLAGS,
    MAVLINK_PARSE_STATE_GOT_SEQ,
    MAVLINK_PARSE_STATE_GOT_SYSID,
    MAVLINK_PARSE_STATE_GOT_COMPID,
    MAVLINK_PARSE_STATE_GOT_MSGID1,
    MAVLINK_PARSE_STATE_GOT_MSGID2,
    MAVLINK_PARSE_STATE_GOT_MSGID3,
    MAVLINK_PARSE_STATE_GOT_PAYLOAD,
    MAVLINK_PARSE_STATE_GOT_CRC1,
    MAVLINK_PARSE_STATE_GOT_CRC2,
    MAVLINK_PARSE_STATE_GOT_BAD_CRC1,
    MAVLINK_PARSE_STATE_SIGNATURE_WAIT,
    MAVLINK_PARSE_STATE_SIGNATURE_WAIT_BAD_CRC
} mavlink_parse_state_t;

typedef enum {
    MAVLINK_FRAMING_INCOMPLETE = 0,
    MAVLINK_FRAMING_OK = 1,
    MAVLINK_FRAMING_BAD_CRC = 2,
    MAVLINK_FRAMING_BAD_SIGNATURE = 3
} mavlink_framing_t;

typedef struct __mavlink_message {
    uint16_t checksum;
    uint8_t magic;
    uint8_t len;
    uint8_t incompat_flags;
    uint8_t compat_flags;
    uint8_t seq;
    uint8_t sysid;
    uint8_t compid;
    uint32_t msgid : 24;
    uint64_t payload64[(MAVLINK_MAX_PAYLOAD_LEN + MAVLINK_NUM_CHECKSUM_BYTES + 7) / 8];
    uint8_t ck[2];
    uint8_t signature[MAVLINK_SIGNATURE_BLOCK_LEN];
} mavlink_message_t;

typedef struct __mavlink_status {
    uint8_t msg_received;
    uint8_t buffer_overrun;
    uint8_t parse_error;
    mavlink_parse_state_t parse_state;
    uint8_t packet_idx;
    uint8_t current_rx_seq;
    uint8_t current_tx_seq;
    uint16_t packet_rx_success_count;
    uint16_t packet_rx_drop_count;
    uint8_t flags;
    uint8_t signature_wait;
    void *signing;
    void *signing_streams;
} mavlink_status_t;
