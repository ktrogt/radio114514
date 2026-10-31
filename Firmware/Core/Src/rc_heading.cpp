#include "rc_internal.hpp"
#include "BNO055.hpp"
#include <cmath>

namespace {
BNO055 bno(&hi2c1, RC_BNO_ADDRESS);
volatile bool sample_due = true;
bool imu_ready = false;
uint32_t init_attempt_ms = 0, settle_until_ms = 0;
unsigned failed_reads = 0;
bool forward_known = false;
bool pid_seeded = false;
float raw_yaw = 0.0f, forward_yaw = 0.0f, target = 0.0f;
float integral = 0.0f, previous_error = 0.0f, correction = 0.0f;
uint16_t previous_count = 0;
float wrap(float degrees) {
    while (degrees >= 180.0f) degrees -= 360.0f;
    while (degrees < -180.0f) degrees += 360.0f;
    return degrees;
}
float clamp(float value, float limit) { return value > limit ? limit : (value < -limit ? -limit : value); }
int correction_limit() {
    if ((rc_state.command.flags & RC_COMMAND_SLOW) != 0U) return RC_CORRECTION_SLOW;
    return (rc_state.command.flags & RC_COMMAND_BOOST) != 0U ? RC_CORRECTION_BOOST : RC_CORRECTION_NORMAL;
}
}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer) {
    if (timer->Instance == TIM6) sample_due = true;
}

// Only a TIM6 start failure is fatal. An IMU that is not ready yet is retried by
// rc_heading_poll() while telemetry keeps reporting the gyro fault.
bool rc_heading_init() {
    imu_ready = bno.init_imu();
    init_attempt_ms = HAL_GetTick();
    settle_until_ms = init_attempt_ms + RC_GYRO_SETTLE_MS;
    sample_due = true;
    return HAL_TIM_Base_Start_IT(&htim6) == HAL_OK;
}

float rc_heading_current() { return forward_known ? wrap(RC_RAW_YAW_TO_CW*(raw_yaw-forward_yaw)) : 0.0f; }
float rc_heading_target() { return target; }

void rc_heading_align() {
    target = rc_heading_current();
    integral = previous_error = correction = 0.0f;
    pid_seeded = false;
}

bool rc_heading_reset() {
    const uint32_t now = HAL_GetTick();
    if (!rc_state.gyro_valid || now-rc_state.last_gyro_ms >= RC_GYRO_TIMEOUT_MS) return false;
    const float previous_heading = rc_heading_current();
    forward_yaw = raw_yaw;
    forward_known = true;
    rc_heading_align();
    rc_line_rebase(previous_heading);
    rc_beep(RC_BEEP_RESET_MS);
    return true;
}

void rc_heading_poll(uint32_t now) {
    bool due;
    { RcCritical lock; due = sample_due; sample_due = false; }
    if (!imu_ready) {
        if (now-init_attempt_ms < RC_GYRO_RETRY_MS) return;
        imu_ready = bno.init_imu();
        init_attempt_ms = now; settle_until_ms = now + RC_GYRO_SETTLE_MS; failed_reads = 0;
        return;
    }
    if (!due || static_cast<int32_t>(now-settle_until_ms) < 0) return;
    EULAR e{};
    if (!bno.read_euler(e)) {
        // A single failed read is tolerated; rc_gate_fault() stops the robot once the
        // last good sample is older than RC_GYRO_TIMEOUT_MS.
        if (++failed_reads >= RC_GYRO_REINIT_FAILURES) { imu_ready = false; init_attempt_ms = now-RC_GYRO_RETRY_MS; }
        return;
    }
    failed_reads = 0;
    raw_yaw = e.z;
    if (!forward_known) { forward_yaw = raw_yaw; forward_known = true; rc_heading_align(); }
    { RcCritical lock; rc_state.last_gyro_ms = now; rc_state.gyro_valid = true; }
    if (!rc_permission(HAL_GetTick())) { rc_heading_align(); return; }
    const uint16_t count = static_cast<uint16_t>(__HAL_TIM_GET_COUNTER(&htim7));
    const float dt = static_cast<float>(static_cast<uint16_t>(count-previous_count));
    const float error = wrap(target-rc_heading_current());
    const float derivative = pid_seeded && dt > 0.0f ? wrap(error-previous_error)/dt : 0.0f;
    integral = RC_GYRO_KI == 0.0f ? 0.0f : clamp(integral+error*dt,RC_GYRO_I_LIMIT);
    correction = RC_GYRO_OUTPUT_SCALE*(error*RC_GYRO_KP + integral*RC_GYRO_KI + derivative*RC_GYRO_KD);
    correction = clamp(correction,static_cast<float>(correction_limit()));
    previous_error = error; previous_count = count; pid_seeded = true;
}

// The turn input is a stick deflection, not a rate (user, 2026-10-14): full deflection
// (the selected speed's RC_TURN_* value) faces RC_TURN_ANGLE_MAX_DEG from forward, and
// releasing the stick turns back to forward. The lead clamp limits how far the target
// may run ahead of the actual heading, so large jumps are approached progressively.
void rc_heading_step() {
    if (!rc_permission(HAL_GetTick())) { rc_heading_align(); return; }
    const uint16_t flags = rc_state.command.flags;
    const int full_scale = (flags & RC_COMMAND_SLOW) != 0U ? RC_TURN_SLOW :
                          ((flags & RC_COMMAND_BOOST) != 0U ? RC_TURN_BOOST : RC_TURN_NORMAL);
    const float deflection = clamp(static_cast<float>(rc_state.command.turn_rate)/static_cast<float>(full_scale),1.0f);
    const float desired = deflection*RC_TURN_ANGLE_MAX_DEG;
    target = wrap(rc_heading_current()+clamp(wrap(desired-rc_heading_current()),RC_TARGET_LEAD_DEG));
}

float rc_heading_correction() { return clamp(correction,static_cast<float>(correction_limit())); }

RcVector rc_field_to_body(RcVector field) {
    const float angle = rc_heading_current()*(3.14159265358979323846f/180.0f);
    const float c = std::cos(angle), s = std::sin(angle);
    return {c*field.x-s*field.y,s*field.x+c*field.y};
}
RcVector rc_body_to_field(RcVector body) {
    const float angle = rc_heading_current()*(3.14159265358979323846f/180.0f);
    const float c = std::cos(angle), s = std::sin(angle);
    return {c*body.x+s*body.y,-s*body.x+c*body.y};
}
