#include "uart_link.h"
#include <Arduino.h>

void UartLink::begin() { Serial1.begin(RC_UART_BAUD, SERIAL_8N1, 44, 43); }
void UartLink::discard() {
    if (used_) { --used_; memmove(bytes_, bytes_ + 1, used_); }
}
bool UartLink::poll(uint32_t now) {
    bool updated = false;
    if (used_ && now - partial_ms_ > 10U) used_ = 0;
    // Bound parser work even on a continuously noisy UART.
    for (unsigned budget = 0; budget < 128 && Serial1.available(); ++budget) {
        if (!used_) partial_ms_ = now;
        if (used_ == sizeof(bytes_)) discard();
        bytes_[used_++] = static_cast<uint8_t>(Serial1.read());
        while (used_) {
            if (bytes_[0] != RC_SYNC_0 || (used_ >= 2 && bytes_[1] != RC_SYNC_1)) { discard(); continue; }
            if (used_ < 5) break;
            if (bytes_[2] != RC_PROTOCOL_VERSION || bytes_[3] != RC_TYPE_TELEMETRY ||
                bytes_[4] != RC_TELEMETRY_PAYLOAD_SIZE) { discard(); continue; }
            if (used_ < RC_TELEMETRY_FRAME_SIZE) break;
            rc_frame_view_t frame{};
            rc_telemetry_t candidate{};
            if (!rc_frame_decode(bytes_, RC_TELEMETRY_FRAME_SIZE, &frame) || !rc_telemetry_decode(&frame, &candidate)) {
                discard(); continue;
            }
            if (!fresh(now) || rc_sequence_newer(frame.sequence, receive_sequence_)) {
                telemetry_ = candidate; receive_sequence_ = frame.sequence; received_ms_ = now;
                seen_ = true; updated = true;
            }
            used_ = 0;
        }
    }
    return updated;
}
void UartLink::send(const rc_command_t& command) {
    uint8_t bytes[RC_COMMAND_FRAME_SIZE];
    const size_t size = rc_command_encode(bytes, sizeof(bytes), send_sequence_++, &command);
    if (size) Serial1.write(bytes, size); // The sole writer to UART1.
}
