#include "rc_internal.hpp"
#include <cmath>

namespace {
bool clear_started = false, clear_qualified = false, normal_known = false, ambiguous = false;
uint32_t clear_since = 0, last_ring_ms = 0;
RcVector field_normal{0.0f,0.0f};
float dot(RcVector a, RcVector b) { return a.x*b.x+a.y*b.y; }
bool feasible(RcVector v, const RcVector *normals, unsigned count) {
    for (unsigned i=0; i<count; ++i) if (dot(v,normals[i]) > RC_VECTOR_EPSILON) return false;
    return true;
}
void update_ring(uint32_t now) {
    const bool ring = (rc_state.line_flags & RC_LINE_RING_MASK) != 0U;
    if (!ring) {
        if (!clear_started) { clear_started = true; clear_since = now; }
        if (now-clear_since >= RC_LINE_CLEAR_MS) { clear_qualified = true; normal_known = false; }
        ambiguous = false;
        return;
    }
    clear_started = false;
    last_ring_ms = now;
    const RcVector raw{static_cast<float>(rc_state.line_x),static_cast<float>(rc_state.line_y)};
    const float magnitude = std::sqrt(dot(raw,raw));
    if (magnitude < RC_LINE_NORMAL_MIN) { ambiguous = !normal_known; return; }
    const RcVector candidate = rc_body_to_field({raw.x/magnitude,raw.y/magnitude});
    if (!normal_known) {
        if (clear_qualified) { field_normal = candidate; normal_known = true; clear_qualified = false; }
    } else if (dot(candidate,field_normal) >= -RC_VECTOR_EPSILON) {
        field_normal = candidate; // R5: only accept directions in the existing hemisphere.
    }
    ambiguous = !normal_known;
}
}

void rc_line_poll(uint32_t now) {
    uint8_t request = 0U, response[3] = {};
    // Bounded match sensor request/response path, preserving the wire encoding.
    bool ok = HAL_UART_AbortReceive(&huart6) == HAL_OK;
    __HAL_UART_CLEAR_PEFLAG(&huart6);
    if (ok) ok = HAL_UART_Transmit(&huart6,&request,1,RC_SENSOR_UART_TIMEOUT_MS) == HAL_OK;
    if (ok) ok = HAL_UART_Receive(&huart6,response,sizeof(response),RC_SENSOR_UART_TIMEOUT_MS) == HAL_OK;
    if (!ok || (response[2] & static_cast<uint8_t>(~RC_LINE_FLAGS_MASK)) != 0U) {
        (void)HAL_UART_AbortReceive(&huart6);
        __HAL_UART_CLEAR_PEFLAG(&huart6);
        // A single failed exchange is tolerated; rc_gate_fault() stops the robot once the
        // last good sample is older than RC_LINE_TIMEOUT_MS while line assist is on.
        return;
    }
    rc_state.line_x = static_cast<int8_t>(static_cast<int16_t>(response[0])-128);
    rc_state.line_y = static_cast<int8_t>(static_cast<int16_t>(response[1])-128);
    rc_state.line_flags = response[2];
    { RcCritical lock; rc_state.last_line_ms = now; rc_state.line_valid = true; }
    update_ring(now);
}

RcVector rc_line_assist(RcVector requested, uint32_t now) {
    if ((rc_state.command.flags & RC_COMMAND_LINE_ASSIST) == 0U) return requested;
    const bool ring = (rc_state.line_flags & RC_LINE_RING_MASK) != 0U;
    if (ring && !normal_known) return {0.0f,0.0f}; // Boot on a line: rotation remains available.
    RcVector normals[4]; unsigned count = 0;
    if (normal_known && (ring || now-last_ring_ms < RC_LINE_HOLD_MS))
        normals[count++] = rc_field_to_body(field_normal);
    if ((rc_state.line_flags & RC_LINE_RIGHT_MASK) != 0U) normals[count++] = {1.0f,0.0f};
    if ((rc_state.line_flags & RC_LINE_LEFT_MASK) != 0U) normals[count++] = {-1.0f,0.0f};
    if ((rc_state.line_flags & RC_LINE_BACK_MASK) != 0U) normals[count++] = {0.0f,-1.0f};
    if (feasible(requested,normals,count)) return requested;
    RcVector best{0.0f,0.0f};
    float best_distance = dot(requested,requested);
    // In 2D every feasible closest point is v, a boundary projection, or zero.
    for (unsigned i=0; i<count; ++i) {
        const float amount = dot(requested,normals[i]);
        const RcVector candidate{requested.x-amount*normals[i].x,requested.y-amount*normals[i].y};
        if (!feasible(candidate,normals,count)) continue;
        const RcVector difference{candidate.x-requested.x,candidate.y-requested.y};
        const float distance = dot(difference,difference);
        if (distance < best_distance) { best = candidate; best_distance = distance; }
    }
    return best;
}

void rc_line_rebase(float previous_heading) {
    if (!normal_known) return;
    const float a = previous_heading*(3.14159265358979323846f/180.0f);
    const float c = std::cos(a), s = std::sin(a);
    field_normal = {c*field_normal.x-s*field_normal.y,s*field_normal.x+c*field_normal.y};
}
bool rc_line_ambiguous() { return ambiguous; }
