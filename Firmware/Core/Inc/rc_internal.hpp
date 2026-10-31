#ifndef RC_INTERNAL_HPP
#define RC_INTERNAL_HPP
#include "rc_state.hpp"

struct RcVector { float x, y; };
uint8_t rc_gate_fault(uint32_t now);
bool rc_permission(uint32_t now);
void rc_stop(uint8_t reason, bool force = false);
void rc_actuators_cutoff();
bool rc_actuators_init();
void rc_actuators_service_stop();
void rc_actuators_park(bool dribbler);
void rc_actuators_apply(RcVector body, float correction, bool dribbler);
void rc_kick_request();
void rc_kick_tick();
void rc_beep(uint32_t duration_ms);
void rc_beep_poll(uint32_t now);
bool rc_heading_init();
void rc_heading_poll(uint32_t now);
void rc_heading_step();
void rc_heading_align();
bool rc_heading_reset();
float rc_heading_current();
float rc_heading_target();
float rc_heading_correction();
RcVector rc_field_to_body(RcVector field);
RcVector rc_body_to_field(RcVector body);
void rc_line_poll(uint32_t now);
RcVector rc_line_assist(RcVector body, uint32_t now);
void rc_line_rebase(float previous_heading);
bool rc_line_ambiguous();
void rc_link_init();
void rc_link_poll(uint32_t now);
void rc_link_telemetry(uint32_t now);
void rc_accept_command(const rc_command_t& command, uint16_t sequence, uint32_t arrival_ms);

#endif
