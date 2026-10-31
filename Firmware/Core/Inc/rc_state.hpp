#ifndef RC_STATE_HPP
#define RC_STATE_HPP

#include "main.h"
#include "rc_protocol.h"
#include "rc_config_f446.h"

/* Foreground owns snapshots; IRQ-visible gate fields are published under PRIMASK. */
struct RcState {
    rc_command_t command{RC_COMMAND_LINE_ASSIST, 0, 0, 0, 0, 0};
    volatile bool ready = false;
    volatile bool red_enabled = false;
    volatile bool rearm_required = true;
    volatile bool dribbler_blocked = true;
    volatile bool wheels_active = false;  // Wheel PWM outputs are running.
    volatile bool wheels_parked = false;  // Drive permitted, but wheel PWM idled while stationary.
    volatile bool dribbler_active = false;
    volatile bool kick_active = false;
    volatile bool have_command = false;
    volatile bool gyro_valid = false;
    volatile bool line_valid = false;
    volatile bool protocol_fault = false;
    volatile bool pairing_window = false;
    volatile bool pending_kick = false;
    volatile bool pending_reset = false;
    volatile uint32_t last_command_ms = 0;
    volatile uint32_t last_gyro_ms = 0;
    volatile uint32_t last_line_ms = 0;
    volatile uint32_t stop_generation = 0;
    volatile uint32_t neutral_after_ms = 0;
    volatile uint32_t uart_errors = 0;
    volatile uint8_t fault = RC_FAULT_INITIALIZATION;
    uint16_t command_sequence = 0;
    uint16_t kick_counter = 0;
    uint16_t reset_counter = 0;
    uint16_t pairing_request = 0;
    int8_t line_x = 0;
    int8_t line_y = 0;
    uint8_t line_flags = 0;
};
extern RcState rc_state;

/* Safe from foreground or IRQ; nested guards preserve the caller's mask. */
class RcCritical {
    uint32_t saved_;
public:
    RcCritical() : saved_(__get_PRIMASK()) { __disable_irq(); __DMB(); }
    ~RcCritical() { __DMB(); __set_PRIMASK(saved_); }
    RcCritical(const RcCritical&) = delete;
    RcCritical& operator=(const RcCritical&) = delete;
};

extern "C" {
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart4, huart6;
extern TIM_HandleTypeDef htim1, htim3, htim6, htim7, htim8, htim13, htim14;
}

#endif
