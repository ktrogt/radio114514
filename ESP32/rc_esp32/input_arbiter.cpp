#include "input_arbiter.h"
#include "gamepad_mapping.h"

bool inputFresh(const VirtualGamepad& input, InputOwner source, uint32_t now) {
    if (!input.connected || !input.valid) return false;
    // HID may report only changes. An approved live BLE link with one valid
    // report remains fresh until Bluepad32 reports a disconnect.
    if (source == InputOwner::Gamepad) return true;
    return source == InputOwner::Browser && input.focused && now - input.received_ms < RC_SOURCE_TIMEOUT_MS;
}

bool InputArbiter::update(const VirtualGamepad& browser, const VirtualGamepad& gamepad, uint32_t now) {
    InputOwner desired = InputOwner::None;
    const VirtualGamepad* candidate = nullptr;
    // A fresh gamepad reserves priority while its non-neutral input awaits qualification.
    if (inputFresh(gamepad, InputOwner::Gamepad, now)) { desired = InputOwner::Gamepad; candidate = &gamepad; }
    else if (inputFresh(browser, InputOwner::Browser, now)) { desired = InputOwner::Browser; candidate = &browser; }
    if (candidate && (owner_ != desired || generation_ != candidate->generation) && !ownershipNeutral(*candidate)) {
        desired = InputOwner::None; candidate = nullptr;
    }
    const uint32_t generation = candidate ? candidate->generation : 0;
    if (owner_ == desired && generation_ == generation) return false;
    owner_ = desired; generation_ = generation; off_sent_ = false; off_ms_ = now;
    return true;
}

void OperatorStop::update(const VirtualGamepad& input, unsigned source, uint32_t now) {
    if (source >= 2 || !input.connected) { if (source < 2) seen_[source] = false; return; }
    if (!seen_[source] || generation_[source] != input.generation) {
        seen_[source] = true; generation_[source] = input.generation;
        counter_[source] = 0; // Adapters start each connection's local stop counter at zero.
    }
    const uint16_t delta = static_cast<uint16_t>(input.stop_press - counter_[source]);
    counter_[source] = input.stop_press;
    if (inputFresh(input, source == 1 ? InputOwner::Gamepad : InputOwner::Browser, now) && delta <= RC_EVENT_MAX_ADVANCE) {
        if (delta >= (active_ ? 2U : 1U)) activation_ = true;
        if (delta & 1U) active_ = !active_;
    }
}
