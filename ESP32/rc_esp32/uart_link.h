#pragma once
#include "rc_protocol.h"

class UartLink {
public:
    void begin();
    bool poll(uint32_t now);
    void send(const rc_command_t& command);
    bool fresh(uint32_t now) const { return seen_ && now - received_ms_ < RC_ESP_TELEMETRY_TIMEOUT_MS; }
    const rc_telemetry_t& telemetry() const { return telemetry_; }
private:
    uint8_t bytes_[RC_MAX_FRAME_SIZE] = {};
    size_t used_ = 0;
    uint32_t partial_ms_ = 0, received_ms_ = 0;
    uint16_t send_sequence_ = 0, receive_sequence_ = 0;
    bool seen_ = false;
    rc_telemetry_t telemetry_{};
    void discard();
};
