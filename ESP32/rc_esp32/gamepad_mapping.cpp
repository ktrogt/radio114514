#include "gamepad_mapping.h"
#include <math.h>

MappedAxes mapAxes(const VirtualGamepad& input) {
    const bool slow = input.lt >= 128;
    const bool boost = !slow && (input.buttons & VG_RB) != 0;
    const int translation = slow ? RC_TRANSLATION_SLOW : boost ? RC_TRANSLATION_BOOST : RC_TRANSLATION_NORMAL;
    const int turn = slow ? RC_TURN_SLOW : boost ? RC_TURN_BOOST : RC_TURN_NORMAL;
    float x = input.lx / 1000.0f, y = input.ly / 1000.0f;
    const float magnitude = sqrtf(x*x + y*y);
    if (magnitude <= 0.12f) { x = 0; y = 0; }
    else {
        const float limited = fminf(magnitude, 1.0f);
        const float scale = (limited - 0.12f) / (0.88f * magnitude);
        x *= scale; y *= scale;
    }
    float rotation = input.rx / 1000.0f;
    constexpr float turn_deadzone = RC_TURN_DEADZONE_PERMILLE / 1000.0f;
    if (fabsf(rotation) <= turn_deadzone) rotation = 0;
    else rotation = copysignf((fminf(fabsf(rotation), 1.0f) - turn_deadzone) / (1.0f - turn_deadzone), rotation);
    // Truncate toward zero: rounded diagonal components can exceed the vector cap.
    return {static_cast<int16_t>(x * translation), static_cast<int16_t>(y * translation),
            static_cast<int16_t>(rotation * turn), boost, slow};
}

bool inputNeutral(const VirtualGamepad& input) {
    const auto axes = mapAxes(input);
    return axes.x == 0 && axes.y == 0 && axes.turn == 0;
}
bool ownershipNeutral(const VirtualGamepad& input) {
    return inputNeutral(input) && input.ry >= -120 && input.ry <= 120;
}

void qualifyReset(VirtualGamepad& input, const VirtualGamepad& previous) {
    input.reset_neutral_press = previous.reset_neutral_press;
    const uint16_t delta = static_cast<uint16_t>(input.reset_press - previous.reset_press);
    if (delta <= RC_EVENT_MAX_ADVANCE && inputNeutral(input)) input.reset_neutral_press += delta;
}

void GamepadMapping::handover(const VirtualGamepad* input) {
    dribbler_ = false;
    line_assist_ = RC_LINE_ASSIST_DEFAULT != 0;
    baseline_ = input ? *input : VirtualGamepad{};
}

rc_command_t GamepadMapping::command(const VirtualGamepad* input, bool enabled, bool force_neutral) {
    rc_command_t result{};
    result.kick_counter = kick_;
    result.reset_counter = reset_;
    if (input) {
        const auto axes = mapAxes(*input);
        const uint16_t kick = static_cast<uint16_t>(input->kick_press - baseline_.kick_press);
        const uint16_t reset = static_cast<uint16_t>(input->reset_neutral_press - baseline_.reset_neutral_press);
        const uint16_t dribble = static_cast<uint16_t>(input->dribbler_press - baseline_.dribbler_press);
        const uint16_t line = static_cast<uint16_t>(input->line_press - baseline_.line_press);
        // Operator-stop frames keep actual axes so only a real neutral input can rearm.
        // A handover explicitly overrides them to neutral for the required off interval.
        if (!force_neutral) {
            result.move_x = axes.x; result.move_y = axes.y; result.turn_rate = axes.turn;
            if (axes.boost) result.flags |= RC_COMMAND_BOOST;
            if (axes.slow) result.flags |= RC_COMMAND_SLOW;
        }
        if (enabled) {
            if (kick <= RC_EVENT_MAX_ADVANCE) kick_ += kick;
            if (reset <= RC_EVENT_MAX_ADVANCE && inputNeutral(*input)) reset_ += reset;
            if (dribble <= RC_EVENT_MAX_ADVANCE && (dribble & 1U)) dribbler_ = !dribbler_;
            if (line <= RC_EVENT_MAX_ADVANCE && (line & 1U)) line_assist_ = !line_assist_;
            result.flags |= RC_COMMAND_ENABLE;
        } else dribbler_ = false;
        // Events during a stop/handover are consumed, never deferred.
        baseline_ = *input;
        result.flags |= RC_COMMAND_SOURCE_ALIVE;
    } else dribbler_ = false;
    if (line_assist_) result.flags |= RC_COMMAND_LINE_ASSIST;
    if (enabled && dribbler_) result.flags |= RC_COMMAND_DRIBBLER;
    result.kick_counter = kick_; result.reset_counter = reset_;
    return result;
}
