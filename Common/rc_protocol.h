#ifndef RC_PROTOCOL_H
#define RC_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "rc_shared_config.h"

/* Frozen protocol v1. No session identifier. Explicit little-endian encoding.
 * Coordinates: +X field right, +Y boot/reset forward, +turn clockwise viewed
 * from above. Translation: per-mille of match full scale. Turn: stick deflection,
 * scaled so the selected speed's RC_TURN_* value is full deflection; the F446 turns
 * that fraction into a target heading relative to forward (since 2026-10-14; the
 * wire format is unchanged, it was a target-heading rate before).
 * CRC-8/ATM: poly 0x07, init 0, non-reflected, xorout 0; covers bytes 2 through
 * payload end, excludes sync and CRC. Test: "123456789" -> 0xF4.
 * UART: 115200 8N1. Commands 50 Hz, telemetry 10 Hz, including disabled frames.
 * First valid command after boot or a >300 ms valid-frame gap baselines event
 * counters WITHOUT action. Counters increment per press and repeat in frames.
 * Duplicate frames never refresh freshness. Events rejected by interlocks or
 * cooldown are consumed, never queued. After any stop, a NEW valid all-zero
 * move/turn frame is required. Dribbler requests must go off then on after a
 * stop; continuous asserted requests cannot restart it. Telemetry reports the
 * actual dribbler state so the ESP can clear its toggle on every stop.
 * BOOST/SLOW select the shared PID/speed limits; SLOW wins if both are asserted.
 * Green short press (<1 s) while outputs disabled resets heading locally.
 * Green hold (3 s) while outputs disabled increments pairing_request once and
 * opens a 30 s disabled window. ESP reacts to counter CHANGE, never its first
 * telemetry value; first BLE controller in that window becomes approved.
 * ESP enable = fresh neutral-qualified owner AND operator stop not active.
 * Source handover sends neutral + enable off >=100 ms, dribbler off, assist on.
 * Example complete golden frames (spaces separate bytes):
 * 1) command seq=0, assist on, disabled neutral, counters=0:
 * A5 5A 01 01 0C 00 00 08 00 00 00 00 00 00 00 00 00 00 00 83
 * 2) command seq=1, alive+enable+assist, forward=400, kick counter=1:
 * A5 5A 01 01 0C 01 00 0B 00 00 00 90 01 00 00 01 00 00 00 C0
 * 3) telemetry seq=0, rearm required, FW=0.1.0, all other fields zero:
 * A5 5A 01 02 1C 00 00 00 08 00 00 00 00 00 00 00 00 00 00 00
 * 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00 EC
 */

#define RC_PROTOCOL_VERSION 1U
#define RC_FIRMWARE_MAJOR 0U
#define RC_FIRMWARE_MINOR 1U
#define RC_FIRMWARE_PATCH 0U
#define RC_UART_BAUD 115200U
#define RC_SYNC_0 0xA5U
#define RC_SYNC_1 0x5AU
#define RC_TYPE_COMMAND 0x01U
#define RC_TYPE_TELEMETRY 0x02U
#define RC_OFFSET_SYNC 0U
#define RC_OFFSET_VERSION 2U
#define RC_OFFSET_TYPE 3U
#define RC_OFFSET_LENGTH 4U
#define RC_OFFSET_SEQUENCE 5U
#define RC_OFFSET_PAYLOAD 7U
#define RC_ENVELOPE_SIZE 8U /* Seven header bytes plus CRC. */
#define RC_MAX_PAYLOAD 32U
#define RC_MAX_FRAME_SIZE (RC_ENVELOPE_SIZE + RC_MAX_PAYLOAD)
#define RC_COMMAND_PAYLOAD_SIZE 12U
#define RC_COMMAND_FRAME_SIZE (RC_ENVELOPE_SIZE + RC_COMMAND_PAYLOAD_SIZE)
#define RC_TELEMETRY_PAYLOAD_SIZE 28U
#define RC_TELEMETRY_FRAME_SIZE (RC_ENVELOPE_SIZE + RC_TELEMETRY_PAYLOAD_SIZE)
#define RC_EVENT_MAX_ADVANCE 8U

#define RC_CMD_FLAGS 0U
#define RC_CMD_MOVE_X 2U
#define RC_CMD_MOVE_Y 4U
#define RC_CMD_TURN 6U
#define RC_CMD_KICK_COUNTER 8U
#define RC_CMD_RESET_COUNTER 10U
#define RC_COMMAND_SOURCE_ALIVE (1U << 0)
#define RC_COMMAND_ENABLE (1U << 1)
#define RC_COMMAND_DRIBBLER (1U << 2)
#define RC_COMMAND_LINE_ASSIST (1U << 3)
#define RC_COMMAND_BOOST (1U << 4)
#define RC_COMMAND_SLOW (1U << 5)
#define RC_COMMAND_FLAGS_MASK 0x003FU

#define RC_TLM_FLAGS 0U
#define RC_TLM_HEADING 2U
#define RC_TLM_TARGET_HEADING 4U
#define RC_TLM_LINE_X 6U
#define RC_TLM_LINE_Y 7U
#define RC_TLM_LINE_FLAGS 8U
#define RC_TLM_FAULT 9U
#define RC_TLM_COMMAND_AGE 10U
#define RC_TLM_KICK_COUNTER 12U
#define RC_TLM_RESET_COUNTER 14U
#define RC_TLM_FW_MAJOR 16U
#define RC_TLM_FW_MINOR 17U
#define RC_TLM_FW_PATCH 18U
#define RC_TLM_RESERVED 19U
#define RC_TLM_COMMAND_SEQUENCE 20U
#define RC_TLM_PAIRING_COUNTER 22U
#define RC_TLM_UART_ERRORS 24U
#define RC_STATUS_RED_ENABLED (1U << 0)
#define RC_STATUS_WHEELS_ACTIVE (1U << 1)
#define RC_STATUS_UART_FRESH (1U << 2)
#define RC_STATUS_SOURCE_FRESH (1U << 3)
#define RC_STATUS_LINE_DETECTED (1U << 4)
#define RC_STATUS_GYRO_HEALTHY (1U << 5)
#define RC_STATUS_LINE_FRESH (1U << 6)
#define RC_STATUS_DRIBBLER_ACTIVE (1U << 7)
#define RC_STATUS_KICK_ACTIVE (1U << 8)
#define RC_STATUS_PAIRING_WINDOW (1U << 9)
#define RC_STATUS_LINE_ASSIST (1U << 10)
#define RC_STATUS_REARM_REQUIRED (1U << 11)
#define RC_STATUS_LINE_AMBIGUOUS (1U << 12)
#define RC_STATUS_FLAGS_MASK 0x1FFFU
#define RC_LINE_RIGHT_MASK 0x03U
#define RC_LINE_BACK_MASK 0x0CU
#define RC_LINE_LEFT_MASK 0x30U
#define RC_LINE_RING_MASK 0x40U
#define RC_LINE_FLAGS_MASK 0x7FU

enum rc_fault {
    RC_FAULT_NONE = 0,
    RC_FAULT_UART_TIMEOUT = 1,
    RC_FAULT_SOURCE = 2,
    RC_FAULT_GYRO = 3,
    RC_FAULT_LINE_STALE = 4,
    RC_FAULT_PROTOCOL = 5,
    RC_FAULT_PAIRING = 6,
    RC_FAULT_INITIALIZATION = 7
};

typedef struct {
    uint16_t flags;
    int16_t move_x, move_y, turn_rate;
    uint16_t kick_counter, reset_counter;
} rc_command_t;

typedef struct {
    uint16_t flags;
    int16_t heading, target_heading;
    int8_t line_x, line_y;
    uint8_t line_flags, fault;
    uint16_t command_age_ms, kick_counter, reset_counter;
    uint8_t firmware_major, firmware_minor, firmware_patch;
    uint16_t command_sequence, pairing_request;
    uint32_t uart_errors;
} rc_telemetry_t;

typedef struct {
    uint8_t type, length;
    uint16_t sequence;
    const uint8_t *payload;
} rc_frame_view_t;

static inline uint16_t rc_read_u16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static inline int16_t rc_read_i16(const uint8_t *p) {
    uint16_t u = rc_read_u16(p);
    return (int16_t)(u <= 32767U ? (int32_t)u : (int32_t)u - 65536);
}
static inline uint32_t rc_read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static inline void rc_write_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static inline void rc_write_i16(uint8_t *p, int16_t v) {
    rc_write_u16(p, (uint16_t)v);
}
static inline void rc_write_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static inline uint8_t rc_crc8(const uint8_t *p, size_t length) {
    uint8_t crc = 0U;
    size_t i;
    for (i = 0U; i < length; ++i) {
        unsigned int bit;
        crc = (uint8_t)(crc ^ p[i]);
        for (bit = 0U; bit < 8U; ++bit)
            crc = (uint8_t)((crc & 0x80U) != 0U ?
                           ((unsigned int)crc << 1) ^ 0x07U :
                           (unsigned int)crc << 1);
    }
    return crc;
}
static inline bool rc_sequence_newer(uint16_t candidate, uint16_t previous) {
    uint16_t delta = (uint16_t)(candidate - previous);
    return delta != 0U && delta < 32768U;
}
static inline bool rc_command_neutral(const rc_command_t *command) {
    return command->move_x == 0 && command->move_y == 0 && command->turn_rate == 0;
}
static inline bool rc_command_valid(const rc_command_t *c) {
    int32_t x = c->move_x, y = c->move_y, turn = c->turn_rate;
    return (c->flags & (uint16_t)~RC_COMMAND_FLAGS_MASK) == 0U &&
           x >= -RC_TRANSLATION_BOOST && x <= RC_TRANSLATION_BOOST &&
           y >= -RC_TRANSLATION_BOOST && y <= RC_TRANSLATION_BOOST &&
           x*x + y*y <= RC_TRANSLATION_BOOST*RC_TRANSLATION_BOOST &&
           turn >= -RC_TURN_BOOST && turn <= RC_TURN_BOOST;
}
static inline bool rc_frame_decode(const uint8_t *bytes, size_t size,
                                   rc_frame_view_t *frame) {
    uint8_t type, length;
    if (bytes == NULL || frame == NULL || size < RC_ENVELOPE_SIZE) return false;
    type = bytes[RC_OFFSET_TYPE]; length = bytes[RC_OFFSET_LENGTH];
    if (bytes[0] != RC_SYNC_0 || bytes[1] != RC_SYNC_1 ||
        bytes[RC_OFFSET_VERSION] != RC_PROTOCOL_VERSION ||
        length > RC_MAX_PAYLOAD || size != RC_ENVELOPE_SIZE + (size_t)length ||
        !((type == RC_TYPE_COMMAND && length == RC_COMMAND_PAYLOAD_SIZE) ||
          (type == RC_TYPE_TELEMETRY && length == RC_TELEMETRY_PAYLOAD_SIZE)) ||
        rc_crc8(bytes + RC_OFFSET_VERSION, size - 3U) != bytes[size - 1U]) return false;
    frame->type = type; frame->length = length;
    frame->sequence = rc_read_u16(bytes + RC_OFFSET_SEQUENCE);
    frame->payload = bytes + RC_OFFSET_PAYLOAD;
    return true;
}
static inline size_t rc_frame_encode(uint8_t *bytes, size_t capacity, uint8_t type,
                                     uint16_t sequence, const uint8_t *payload,
                                     uint8_t length) {
    size_t size = RC_ENVELOPE_SIZE + (size_t)length;
    if (bytes == NULL || payload == NULL || capacity < size ||
        !((type == RC_TYPE_COMMAND && length == RC_COMMAND_PAYLOAD_SIZE) ||
          (type == RC_TYPE_TELEMETRY && length == RC_TELEMETRY_PAYLOAD_SIZE))) return 0U;
    bytes[0] = RC_SYNC_0; bytes[1] = RC_SYNC_1;
    bytes[2] = RC_PROTOCOL_VERSION; bytes[3] = type; bytes[4] = length;
    rc_write_u16(bytes + RC_OFFSET_SEQUENCE, sequence);
    memcpy(bytes + RC_OFFSET_PAYLOAD, payload, length);
    bytes[size - 1U] = rc_crc8(bytes + 2U, size - 3U);
    return size;
}
static inline bool rc_command_decode(const rc_frame_view_t *frame, rc_command_t *c) {
    const uint8_t *p;
    if (frame == NULL || c == NULL || frame->type != RC_TYPE_COMMAND ||
        frame->length != RC_COMMAND_PAYLOAD_SIZE || frame->payload == NULL) return false;
    p = frame->payload;
    c->flags = rc_read_u16(p + RC_CMD_FLAGS);
    c->move_x = rc_read_i16(p + RC_CMD_MOVE_X);
    c->move_y = rc_read_i16(p + RC_CMD_MOVE_Y);
    c->turn_rate = rc_read_i16(p + RC_CMD_TURN);
    c->kick_counter = rc_read_u16(p + RC_CMD_KICK_COUNTER);
    c->reset_counter = rc_read_u16(p + RC_CMD_RESET_COUNTER);
    return rc_command_valid(c);
}
static inline size_t rc_command_encode(uint8_t *bytes, size_t capacity,
                                       uint16_t sequence, const rc_command_t *c) {
    uint8_t p[RC_COMMAND_PAYLOAD_SIZE];
    if (c == NULL || !rc_command_valid(c)) return 0U;
    rc_write_u16(p + RC_CMD_FLAGS, c->flags);
    rc_write_i16(p + RC_CMD_MOVE_X, c->move_x);
    rc_write_i16(p + RC_CMD_MOVE_Y, c->move_y);
    rc_write_i16(p + RC_CMD_TURN, c->turn_rate);
    rc_write_u16(p + RC_CMD_KICK_COUNTER, c->kick_counter);
    rc_write_u16(p + RC_CMD_RESET_COUNTER, c->reset_counter);
    return rc_frame_encode(bytes, capacity, RC_TYPE_COMMAND, sequence, p, sizeof(p));
}
static inline size_t rc_telemetry_encode(uint8_t *bytes, size_t capacity,
                                         uint16_t sequence, const rc_telemetry_t *t) {
    uint8_t p[RC_TELEMETRY_PAYLOAD_SIZE] = {0};
    if (t == NULL) return 0U;
    rc_write_u16(p + RC_TLM_FLAGS, t->flags);
    rc_write_i16(p + RC_TLM_HEADING, t->heading);
    rc_write_i16(p + RC_TLM_TARGET_HEADING, t->target_heading);
    p[RC_TLM_LINE_X] = (uint8_t)t->line_x; p[RC_TLM_LINE_Y] = (uint8_t)t->line_y;
    p[RC_TLM_LINE_FLAGS] = t->line_flags; p[RC_TLM_FAULT] = t->fault;
    rc_write_u16(p + RC_TLM_COMMAND_AGE, t->command_age_ms);
    rc_write_u16(p + RC_TLM_KICK_COUNTER, t->kick_counter);
    rc_write_u16(p + RC_TLM_RESET_COUNTER, t->reset_counter);
    p[RC_TLM_FW_MAJOR] = t->firmware_major; p[RC_TLM_FW_MINOR] = t->firmware_minor;
    p[RC_TLM_FW_PATCH] = t->firmware_patch;
    rc_write_u16(p + RC_TLM_COMMAND_SEQUENCE, t->command_sequence);
    rc_write_u16(p + RC_TLM_PAIRING_COUNTER, t->pairing_request);
    rc_write_u32(p + RC_TLM_UART_ERRORS, t->uart_errors);
    return rc_frame_encode(bytes, capacity, RC_TYPE_TELEMETRY, sequence, p, sizeof(p));
}
static inline bool rc_telemetry_decode(const rc_frame_view_t *frame, rc_telemetry_t *t) {
    const uint8_t *p;
    if (frame == NULL || t == NULL || frame->type != RC_TYPE_TELEMETRY ||
        frame->length != RC_TELEMETRY_PAYLOAD_SIZE || frame->payload == NULL) return false;
    p = frame->payload;
    t->flags = rc_read_u16(p + RC_TLM_FLAGS);
    t->heading = rc_read_i16(p + RC_TLM_HEADING);
    t->target_heading = rc_read_i16(p + RC_TLM_TARGET_HEADING);
    t->line_x = (int8_t)(p[RC_TLM_LINE_X] <= 127U ? (int)p[RC_TLM_LINE_X] : (int)p[RC_TLM_LINE_X] - 256);
    t->line_y = (int8_t)(p[RC_TLM_LINE_Y] <= 127U ? (int)p[RC_TLM_LINE_Y] : (int)p[RC_TLM_LINE_Y] - 256);
    t->line_flags = p[RC_TLM_LINE_FLAGS]; t->fault = p[RC_TLM_FAULT];
    t->command_age_ms = rc_read_u16(p + RC_TLM_COMMAND_AGE);
    t->kick_counter = rc_read_u16(p + RC_TLM_KICK_COUNTER);
    t->reset_counter = rc_read_u16(p + RC_TLM_RESET_COUNTER);
    t->firmware_major = p[RC_TLM_FW_MAJOR]; t->firmware_minor = p[RC_TLM_FW_MINOR];
    t->firmware_patch = p[RC_TLM_FW_PATCH];
    t->command_sequence = rc_read_u16(p + RC_TLM_COMMAND_SEQUENCE);
    t->pairing_request = rc_read_u16(p + RC_TLM_PAIRING_COUNTER);
    t->uart_errors = rc_read_u32(p + RC_TLM_UART_ERRORS);
    return (t->flags & (uint16_t)~RC_STATUS_FLAGS_MASK) == 0U &&
           (t->line_flags & (uint8_t)~RC_LINE_FLAGS_MASK) == 0U && p[RC_TLM_RESERVED] == 0U &&
           t->fault <= RC_FAULT_INITIALIZATION;
}

#endif
