#include "rc_api.h"
#include "rc_internal.hpp"

uint8_t rc_gate_fault(uint32_t now) {
    if (!rc_state.ready) return RC_FAULT_INITIALIZATION;
    if (rc_state.protocol_fault) return RC_FAULT_PROTOCOL;
    if (rc_state.pairing_window) return RC_FAULT_PAIRING;
    if (!rc_state.have_command || now-rc_state.last_command_ms >= RC_UART_TIMEOUT_MS)
        return RC_FAULT_UART_TIMEOUT;
    if ((rc_state.command.flags & RC_COMMAND_SOURCE_ALIVE) == 0U ||
        (rc_state.command.flags & RC_COMMAND_ENABLE) == 0U) return RC_FAULT_SOURCE;
    if (!rc_state.gyro_valid || now-rc_state.last_gyro_ms >= RC_GYRO_TIMEOUT_MS)
        return RC_FAULT_GYRO;
    if ((rc_state.command.flags & RC_COMMAND_LINE_ASSIST) != 0U &&
        (!rc_state.line_valid || now-rc_state.last_line_ms >= RC_LINE_TIMEOUT_MS))
        return RC_FAULT_LINE_STALE;
    return RC_FAULT_NONE;
}

bool rc_permission(uint32_t now) {
    return rc_state.red_enabled && !rc_state.rearm_required && rc_gate_fault(now) == RC_FAULT_NONE;
}

void rc_stop(uint8_t reason, bool force) {
    RcCritical lock;
    const bool transition = force || !rc_state.rearm_required || rc_state.wheels_active || rc_state.wheels_parked ||
                            rc_state.dribbler_active || rc_state.kick_active || rc_state.fault != reason;
    rc_actuators_cutoff();
    rc_state.rearm_required = true;
    rc_state.pending_kick = false;
    rc_state.pending_reset = false;
    rc_state.fault = reason;
    if (transition) {
        rc_state.dribbler_blocked = true;
        rc_state.neutral_after_ms = HAL_GetTick();
        ++rc_state.stop_generation;
    }
}

extern "C" void rc_safety_tick(void) {
    if (!rc_state.ready) return;
    rc_kick_tick(); // Bound the pulse even if foreground sensor code stops progressing.
    const uint8_t fault = rc_gate_fault(HAL_GetTick());
    if (fault != RC_FAULT_NONE || !rc_state.red_enabled) rc_stop(fault);
}

extern "C" void rc_emergency_cutoff(void) {
    __disable_irq();
    rc_actuators_cutoff();
    rc_state.ready = false;
    rc_state.rearm_required = true;
    rc_state.dribbler_blocked = true;
    rc_state.pending_kick = false;
    rc_state.pending_reset = false;
    // Fatal entry deliberately leaves interrupts masked. No HAL/timeouts here.
}
