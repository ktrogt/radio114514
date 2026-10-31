#include "rc_api.h"
#include "rc_internal.hpp"
#include <cmath>

RcState rc_state;
namespace {
uint32_t last_control = 0, last_telemetry = 0, serviced_stop = 0;
struct Switch {
    bool candidate = false, stable = false;
    uint32_t changed_ms = 0;
    bool update(bool raw,uint32_t now) {
        if (raw != candidate) { candidate = raw; changed_ms = now; }
        if (stable != candidate && now-changed_ms >= RC_SWITCH_DEBOUNCE_MS) { stable = candidate; return true; }
        return false;
    }
};
Switch red, green;
uint32_t green_started = 0, pairing_started = 0;
bool green_eligible = false, green_long_done = false;
uint32_t settled_since = 0;
bool settling = false;
float parked_target = 0.0f;
// Driving is permitted, whether the wheel PWM is running or parked.
bool driving() { return rc_state.wheels_active || rc_state.wheels_parked; }
float wrapped_abs(float degrees) {
    while (degrees >= 180.0f) degrees -= 360.0f;
    while (degrees < -180.0f) degrees += 360.0f;
    return std::fabs(degrees);
}
// Idle the wheel PWM while stationary: 50 % locked-antiphase drive heats the motors.
// Park after translation has been zero and the heading within RC_PARK_ERROR_DEG of its
// target for RC_PARK_HOLD_MS (also while a turn stick holds an angle). Wake on any
// translation, when the target angle moves (turn stick moved), or when pushed beyond
// RC_UNPARK_ERROR_DEG.
bool should_park(uint32_t now) {
    if (rc_state.command.move_x != 0 || rc_state.command.move_y != 0) { settling = false; return false; }
    const float error = wrapped_abs(rc_heading_target()-rc_heading_current());
    if (rc_state.wheels_parked) {
        if (error <= RC_UNPARK_ERROR_DEG &&
            wrapped_abs(rc_heading_target()-parked_target) <= RC_PARK_ERROR_DEG) return true;
        settling = false; return false;
    }
    if (error > RC_PARK_ERROR_DEG) { settling = false; return false; }
    if (!settling) { settling = true; settled_since = now; }
    if (now-settled_since < RC_PARK_HOLD_MS) return false;
    parked_target = rc_heading_target();
    return true;
}
void switches(uint32_t now) {
    // PC14/PC15 PLAY inputs are intentionally never read by RC control.
    const bool red_edge = red.update(HAL_GPIO_ReadPin(SW2_GPIO_Port,SW2_Pin) == GPIO_PIN_SET,now);
    if (red_edge && red.stable) {
        RcCritical lock;
        rc_state.red_enabled = !rc_state.red_enabled;
        rc_stop(RC_FAULT_NONE,true); // Both directions require a subsequently received neutral frame.
        if (rc_state.red_enabled) rc_beep(RC_BEEP_ENABLE_MS);
    }
    const bool green_edge = green.update(HAL_GPIO_ReadPin(SW3_GPIO_Port,SW3_Pin) == GPIO_PIN_SET,now);
    if (green_edge && green.stable) {
        green_started = now; green_eligible = !driving(); green_long_done = false;
    }
    if (green.stable && driving()) green_eligible = false;
    if (green.stable && green_eligible && !green_long_done && now-green_started >= RC_GREEN_HOLD_MS) {
        green_long_done = true; pairing_started = now;
        { RcCritical lock; ++rc_state.pairing_request; rc_state.pairing_window = true; rc_stop(RC_FAULT_PAIRING,true); }
        rc_beep(RC_BEEP_PAIR_MS);
    }
    if (green_edge && !green.stable && green_eligible && !green_long_done &&
        now-green_started < RC_GREEN_SHORT_MS && !driving()) (void)rc_heading_reset();
    if (rc_state.pairing_window && now-pairing_started >= RC_PAIRING_WINDOW_MS) {
        RcCritical lock; rc_state.pairing_window = false; rc_stop(RC_FAULT_NONE,true);
    }
}
RcVector limited_field_command() {
    const uint16_t flags = rc_state.command.flags;
    const float limit = static_cast<float>((flags & RC_COMMAND_SLOW) != 0U ? RC_TRANSLATION_SLOW :
                         ((flags & RC_COMMAND_BOOST) != 0U ? RC_TRANSLATION_BOOST : RC_TRANSLATION_NORMAL));
    RcVector v{static_cast<float>(rc_state.command.move_x),static_cast<float>(rc_state.command.move_y)};
    const float magnitude = std::sqrt(v.x*v.x+v.y*v.y);
    if (magnitude > limit) { v.x *= limit/magnitude; v.y *= limit/magnitude; }
    return v;
}
void service_stop() {
    if (serviced_stop != rc_state.stop_generation || (!rc_state.wheels_active && rc_state.rearm_required)) {
        RcCritical lock;
        rc_actuators_service_stop(); rc_heading_align(); serviced_stop = rc_state.stop_generation;
    }
}
}

extern "C" void rc_init(void) {
    rc_actuators_cutoff();
    const bool hardware_ok = HAL_TIM_Base_Start(&htim7) == HAL_OK && rc_actuators_init();
    HAL_Delay(RC_GYRO_BOOT_MS); // Let the BNO055 finish booting before the IMU-mode write.
    const bool heading_ok = rc_heading_init();
    last_control = last_telemetry = HAL_GetTick();
    rc_state.neutral_after_ms = last_control;
    rc_link_init();
    RcCritical lock;
    if (!hardware_ok || !heading_ok) {
        // Never halt silently: outputs stay cut off and telemetry keeps reporting the fault.
        rc_state.ready = false; rc_state.rearm_required = true; rc_state.fault = RC_FAULT_INITIALIZATION;
        return;
    }
    rc_state.ready = true; rc_stop(RC_FAULT_UART_TIMEOUT,true);
}

extern "C" void rc_poll(void) {
    const uint32_t now = HAL_GetTick();
    rc_link_poll(now);
    service_stop();
    if (rc_state.ready && now-last_control >= RC_CONTROL_PERIOD_MS) {
        last_control = now;
        switches(now);
        rc_heading_poll(HAL_GetTick());
        rc_line_poll(HAL_GetTick());
        const uint32_t current = HAL_GetTick();
        const uint8_t fault = rc_gate_fault(current);
        if (fault != RC_FAULT_NONE || !rc_state.red_enabled) rc_stop(fault);
        service_stop();
        if (rc_permission(current)) {
            bool reset, kick;
            { RcCritical lock; reset = rc_state.pending_reset; kick = rc_state.pending_kick;
              rc_state.pending_reset = false; rc_state.pending_kick = false; }
            if (reset && rc_command_neutral(&rc_state.command)) (void)rc_heading_reset();
            rc_heading_step();
            const RcVector body = rc_line_assist(rc_field_to_body(limited_field_command()),current);
            const bool dribbler = (rc_state.command.flags & RC_COMMAND_DRIBBLER) != 0U;
            if (should_park(current)) rc_actuators_park(dribbler);
            else rc_actuators_apply(body,rc_heading_correction(),dribbler);
            if (kick) rc_kick_request();
        } else { settling = false; rc_heading_align(); }
    }
    rc_beep_poll(HAL_GetTick());
    const uint32_t telemetry_now = HAL_GetTick();
    if (telemetry_now-last_telemetry >= 1000U/RC_TELEMETRY_HZ) {
        last_telemetry = telemetry_now; rc_link_telemetry(telemetry_now);
    }
}
