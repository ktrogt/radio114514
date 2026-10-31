#pragma once
#include "rc_protocol.h"
#include "virtual_gamepad.h"

struct MappedAxes { int16_t x, y, turn; bool boost, slow; };
MappedAxes mapAxes(const VirtualGamepad& input);
bool inputNeutral(const VirtualGamepad& input);
bool ownershipNeutral(const VirtualGamepad& input);
void qualifyReset(VirtualGamepad& input, const VirtualGamepad& previous);

class GamepadMapping {
public:
    void handover(const VirtualGamepad* input);
    void clearDribbler() { dribbler_ = false; }
    rc_command_t command(const VirtualGamepad* input, bool enabled, bool force_neutral);
    bool dribbler() const { return dribbler_; }
    bool lineAssist() const { return line_assist_; }
private:
    VirtualGamepad baseline_;
    uint16_t kick_ = 0, reset_ = 0;
    bool dribbler_ = false, line_assist_ = RC_LINE_ASSIST_DEFAULT != 0;
};
