#pragma once
#include "virtual_gamepad.h"
#include "rc_shared_config.h"

bool inputFresh(const VirtualGamepad& input, InputOwner source, uint32_t now);
class InputArbiter {
public:
    bool update(const VirtualGamepad& browser, const VirtualGamepad& gamepad, uint32_t now);
    InputOwner owner() const { return owner_; }
    bool ready(uint32_t now) const { return owner_ != InputOwner::None && off_sent_ && now - off_ms_ >= RC_OWNER_CHANGE_OFF_MS; }
    void disabledFrameSent(uint32_t now) { if (!off_sent_) { off_ms_ = now; off_sent_ = true; } }
private:
    InputOwner owner_ = InputOwner::None;
    uint32_t generation_ = 0, off_ms_ = 0;
    bool off_sent_ = false;
};

// Stops remain available from either fresh source, including the page during BLE ownership.
class OperatorStop {
public:
    void update(const VirtualGamepad& input, unsigned source, uint32_t now);
    bool active() const { return active_; }
    bool consumeActivation() { const bool value = activation_; activation_ = false; return value; }
private:
    uint32_t generation_[2] = {};
    uint16_t counter_[2] = {};
    bool seen_[2] = {}, active_ = false, activation_ = false;
};
