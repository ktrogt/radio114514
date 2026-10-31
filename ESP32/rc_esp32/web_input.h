#pragma once
#include "virtual_gamepad.h"
#include "rc_protocol.h"

struct PageStatus {
    InputOwner owner = InputOwner::None;
    bool operator_stop = false, robot_link = false, handover = false;
    bool gamepad_connected = false, gamepad_approved = false, pairing_saved = true;
    uint32_t pairing_seconds = 0;
    char gamepad_model[80] = {}, gamepad_address[18] = {};
    uint16_t command_flags = RC_COMMAND_LINE_ASSIST;
    rc_telemetry_t robot{};
};
namespace WebInput {
void begin();
void service();
VirtualGamepad snapshot();
void publish(const PageStatus& status);
}
