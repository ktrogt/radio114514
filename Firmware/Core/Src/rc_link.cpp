#include "rc_internal.hpp"
#include <cstring>

namespace {
uint8_t rx_byte = 0;
uint8_t ring[RC_UART_RING_SIZE];
uint32_t arrival[RC_UART_RING_SIZE];
volatile uint16_t head = 0, tail = 0;
volatile bool overflow = false;
uint8_t parser[RC_MAX_FRAME_SIZE];
size_t used = 0;
uint32_t previous_byte_ms = 0;
uint8_t tx[RC_TELEMETRY_FRAME_SIZE];
volatile bool tx_busy = false;
uint16_t telemetry_sequence = 0;
static_assert((RC_UART_RING_SIZE & (RC_UART_RING_SIZE-1U)) == 0U,"RX ring must be a power of two");
void note_error() { RcCritical lock; ++rc_state.uart_errors; }
void discard_front() {
    if (used != 0U) { --used; std::memmove(parser,parser+1,used); }
}
void parse_byte(uint8_t byte, uint32_t stamp) {
    if (used != 0U && stamp-previous_byte_ms > RC_FRAME_TIMEOUT_MS) { used = 0; note_error(); }
    previous_byte_ms = stamp;
    if (used >= sizeof(parser)) { discard_front(); note_error(); }
    parser[used++] = byte;
    while (used != 0U) {
        if (parser[0] != RC_SYNC_0) { discard_front(); continue; }
        if (used < 2U) return;
        if (parser[1] != RC_SYNC_1) { discard_front(); continue; }
        if (used < RC_OFFSET_PAYLOAD) return;
        if (parser[RC_OFFSET_VERSION] != RC_PROTOCOL_VERSION ||
            parser[RC_OFFSET_TYPE] != RC_TYPE_COMMAND || parser[RC_OFFSET_LENGTH] != RC_COMMAND_PAYLOAD_SIZE) {
            note_error(); rc_state.protocol_fault = true; rc_stop(RC_FAULT_PROTOCOL);
            discard_front(); continue;
        }
        if (used < RC_COMMAND_FRAME_SIZE) return;
        rc_frame_view_t frame{}; rc_command_t command{};
        if (rc_frame_decode(parser,RC_COMMAND_FRAME_SIZE,&frame) && rc_command_decode(&frame,&command)) {
            rc_accept_command(command,frame.sequence,stamp);
            used -= RC_COMMAND_FRAME_SIZE;
            std::memmove(parser,parser+RC_COMMAND_FRAME_SIZE,used);
        } else {
            note_error(); rc_state.protocol_fault = true; rc_stop(RC_FAULT_PROTOCOL);
            discard_front();
        }
    }
}
int16_t heading_wire(float heading) { return static_cast<int16_t>(heading*100.0f); }
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart) {
    if (uart->Instance != UART4) return;
    const uint16_t next = static_cast<uint16_t>((head+1U)&(RC_UART_RING_SIZE-1U));
    if (next == tail) {
        overflow = true; ++rc_state.uart_errors; rc_state.protocol_fault = true;
        rc_stop(RC_FAULT_PROTOCOL);
    } else {
        ring[head] = rx_byte; arrival[head] = HAL_GetTick(); __DMB(); head = next;
    }
    if (HAL_UART_Receive_IT(uart,&rx_byte,1) != HAL_OK) {
        ++rc_state.uart_errors; rc_state.protocol_fault = true; rc_stop(RC_FAULT_PROTOCOL);
    }
}
extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart) {
    if (uart->Instance != UART4) return;
    ++rc_state.uart_errors; rc_state.protocol_fault = true; overflow = true;
    rc_stop(RC_FAULT_PROTOCOL);
    (void)HAL_UART_AbortReceive(uart);
    __HAL_UART_CLEAR_PEFLAG(uart);
    (void)HAL_UART_Receive_IT(uart,&rx_byte,1);
}
extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart) {
    if (uart->Instance == UART4) tx_busy = false;
}

void rc_link_init() {
    head = tail = 0; used = 0; overflow = false;
    __HAL_UART_CLEAR_PEFLAG(&huart4);
    if (HAL_UART_Receive_IT(&huart4,&rx_byte,1) != HAL_OK) {
        rc_state.protocol_fault = true; note_error();
    }
}
void rc_link_poll(uint32_t now) {
    {
        RcCritical lock;
        if (overflow) { tail = head; used = 0; overflow = false; }
    }
    if (huart4.RxState == HAL_UART_STATE_READY && HAL_UART_Receive_IT(&huart4,&rx_byte,1) != HAL_OK) {
        rc_state.protocol_fault = true; note_error(); rc_stop(RC_FAULT_PROTOCOL);
    }
    while (tail != head) {
        const uint16_t index = tail;
        const uint8_t byte = ring[index]; const uint32_t stamp = arrival[index];
        __DMB(); tail = static_cast<uint16_t>((index+1U)&(RC_UART_RING_SIZE-1U));
        parse_byte(byte,stamp);
    }
    // Bytes parsed above may be newer than the entry-time `now`; compare against a fresh tick.
    (void)now;
    if (used != 0U && HAL_GetTick()-previous_byte_ms > RC_FRAME_TIMEOUT_MS) { used = 0; note_error(); }
    // On a transmit error, recover the persistent buffer after HAL releases it.
    if (tx_busy && huart4.gState == HAL_UART_STATE_READY) tx_busy = false;
}

void rc_accept_command(const rc_command_t& command, uint16_t sequence, uint32_t stamp) {
    const uint32_t now = HAL_GetTick();
    if (now-stamp >= RC_UART_TIMEOUT_MS) return; // Queued old bytes cannot renew the watchdog.
    RcCritical lock;
    const bool baseline = !rc_state.have_command || now-rc_state.last_command_ms > RC_UART_TIMEOUT_MS;
    if (!baseline && !rc_sequence_newer(sequence,rc_state.command_sequence)) return;
    const uint16_t kicks = static_cast<uint16_t>(command.kick_counter-rc_state.kick_counter);
    const uint16_t resets = static_cast<uint16_t>(command.reset_counter-rc_state.reset_counter);
    if (!baseline && (kicks > RC_EVENT_MAX_ADVANCE || resets > RC_EVENT_MAX_ADVANCE)) {
        ++rc_state.uart_errors; rc_state.protocol_fault = true; rc_stop(RC_FAULT_PROTOCOL); return;
    }
    const bool was_rearming = rc_state.rearm_required;
    rc_state.command = command; rc_state.command_sequence = sequence;
    rc_state.last_command_ms = stamp; rc_state.have_command = true; rc_state.protocol_fault = false;
    rc_state.kick_counter = command.kick_counter; rc_state.reset_counter = command.reset_counter;
    if ((command.flags & RC_COMMAND_DRIBBLER) == 0U) rc_state.dribbler_blocked = false;
    const uint8_t fault = rc_gate_fault(now);
    if (fault != RC_FAULT_NONE || !rc_state.red_enabled) { rc_stop(fault); return; }
    rc_state.fault = RC_FAULT_NONE;
    if (rc_state.rearm_required && rc_command_neutral(&command) &&
        static_cast<int32_t>(stamp-rc_state.neutral_after_ms) > 0) rc_state.rearm_required = false;
    if (!baseline && !was_rearming && rc_permission(now)) {
        if (kicks != 0U) rc_state.pending_kick = true;
        if (resets != 0U && rc_command_neutral(&command)) rc_state.pending_reset = true;
    }
}

void rc_link_telemetry(uint32_t now) {
    if (tx_busy) return;
    rc_telemetry_t t{};
    {
        RcCritical lock;
        const bool uart_fresh = rc_state.have_command && now-rc_state.last_command_ms < RC_UART_TIMEOUT_MS;
        const bool gyro_fresh = rc_state.gyro_valid && now-rc_state.last_gyro_ms < RC_GYRO_TIMEOUT_MS;
        const bool line_fresh = rc_state.line_valid && now-rc_state.last_line_ms < RC_LINE_TIMEOUT_MS;
        if (rc_state.red_enabled) t.flags |= RC_STATUS_RED_ENABLED;
        // "Driving permitted": a parked (PWM-idled) robot still reports active so the page and ESP don't treat it as stopped.
        if (rc_state.wheels_active || rc_state.wheels_parked) t.flags |= RC_STATUS_WHEELS_ACTIVE;
        if (uart_fresh) t.flags |= RC_STATUS_UART_FRESH;
        if (uart_fresh && (rc_state.command.flags & RC_COMMAND_SOURCE_ALIVE) != 0U) t.flags |= RC_STATUS_SOURCE_FRESH;
        if ((rc_state.line_flags & RC_LINE_FLAGS_MASK) != 0U) t.flags |= RC_STATUS_LINE_DETECTED;
        if (gyro_fresh) t.flags |= RC_STATUS_GYRO_HEALTHY;
        if (line_fresh) t.flags |= RC_STATUS_LINE_FRESH;
        if (rc_state.dribbler_active) t.flags |= RC_STATUS_DRIBBLER_ACTIVE;
        if (rc_state.kick_active) t.flags |= RC_STATUS_KICK_ACTIVE;
        if (rc_state.pairing_window) t.flags |= RC_STATUS_PAIRING_WINDOW;
        if ((rc_state.command.flags & RC_COMMAND_LINE_ASSIST) != 0U) t.flags |= RC_STATUS_LINE_ASSIST;
        if (rc_state.rearm_required) t.flags |= RC_STATUS_REARM_REQUIRED;
        if (rc_line_ambiguous()) t.flags |= RC_STATUS_LINE_AMBIGUOUS;
        t.heading = heading_wire(rc_heading_current()); t.target_heading = heading_wire(rc_heading_target());
        t.line_x = rc_state.line_x; t.line_y = rc_state.line_y; t.line_flags = rc_state.line_flags;
        t.fault = rc_state.fault;
        const uint32_t age = rc_state.have_command ? now-rc_state.last_command_ms : 65535U;
        t.command_age_ms = static_cast<uint16_t>(age > 65535U ? 65535U : age);
        t.kick_counter = rc_state.kick_counter; t.reset_counter = rc_state.reset_counter;
        t.command_sequence = rc_state.command_sequence; t.pairing_request = rc_state.pairing_request;
        t.uart_errors = rc_state.uart_errors;
    }
    t.firmware_major = RC_FIRMWARE_MAJOR; t.firmware_minor = RC_FIRMWARE_MINOR; t.firmware_patch = RC_FIRMWARE_PATCH;
    if (rc_telemetry_encode(tx,sizeof(tx),telemetry_sequence++,&t) == 0U) return;
    RcCritical lock;
    tx_busy = true;
    if (HAL_UART_Transmit_IT(&huart4,tx,sizeof(tx)) != HAL_OK) { tx_busy = false; ++rc_state.uart_errors; }
}
