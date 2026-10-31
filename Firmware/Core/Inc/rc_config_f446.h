#ifndef RC_CONFIG_F446_H
#define RC_CONFIG_F446_H

/* All F446-only tunables; shared motion limits are in rc_shared_config.h. */
#define RC_CONTROL_PERIOD_MS 10U /* Foreground actuator/control update: 100 Hz. */
#define RC_UART_TIMEOUT_MS 300U /* Stop at 300 ms; counters rebaseline after a greater gap. */
#define RC_GYRO_TIMEOUT_MS 100U /* Maximum age of a successfully checked yaw sample. */
#define RC_LINE_TIMEOUT_MS 100U /* Maximum line age while line assist is enabled. */
#define RC_FRAME_TIMEOUT_MS 10U /* Abandon an incomplete UART frame after this inter-byte gap. */
#define RC_UART_RING_SIZE 256U /* Power-of-two interrupt receive ring capacity. */
#define RC_SENSOR_UART_TIMEOUT_MS 2U /* Per-operation bound on line request and response. */
#define RC_I2C_TIMEOUT_MS 10U /* Per-operation bound on BNO055 transactions (100 kHz bus, BNO055 clock-stretches). */
#define RC_GYRO_SETTLE_MS 50U /* Wait after the original IMU-mode register write. */
#define RC_GYRO_BOOT_MS 700U /* BNO055 needs about 650 ms after power-on; the match firmware waited 500 ms. */
#define RC_GYRO_RETRY_MS 500U /* Retry interval while the IMU-mode write keeps failing. */
#define RC_GYRO_REINIT_FAILURES 25U /* Consecutive failed reads (~0.5 s) before re-sending the IMU-mode write. */
#define RC_BNO_ADDRESS 0x28U /* Existing seven-bit BNO055 I2C address. */
#define RC_GYRO_KP 0.6f /* Match straight-heading proportional gain. */
#define RC_GYRO_KI 0.0f /* Match straight-heading integral gain. */
#define RC_GYRO_KD 1.0f /* Match derivative gain with dt measured in TIM7 counts. */
#define RC_GYRO_I_LIMIT 10000.0f /* Integral clamp, including when nonzero Ki is selected later. */
#define RC_GYRO_OUTPUT_SCALE (-18.0f) /* Preserve match PID-to-chassis correction polarity/scale. */
#define RC_RAW_YAW_TO_CW (-1.0f) /* Inferred BNO yaw sign; physically confirm before powered motion. */
#define RC_TURN_ANGLE_MAX_DEG 45.0f /* Full turn-stick deflection faces this far from forward (was 90; user, 2026-10-15). */
#define RC_PARK_ERROR_DEG 2.0f /* Idle wheel PWM when translation is zero and |heading error| stays within this... */
#define RC_PARK_HOLD_MS 300U /* ...for this long (motors otherwise heat up under 50 % locked-antiphase drive). */
#define RC_UNPARK_ERROR_DEG 5.0f /* Restart wheel PWM to hold heading when the robot is turned beyond this. */
#define RC_TARGET_LEAD_DEG 60.0f /* Limit accumulated heading demand; 60 deg lets Kp reach the 640 normal cap (was 30). */
#define RC_MATCH_TRANSLATION_FULL 900.0f /* Match mv_power=100 conversion into chassis magnitude. */
#define RC_WHEEL_COMPARE_MARGIN 10U /* Match margin below either half-period compare boundary. */
#define RC_PROFILE_SIRO 0 /* Hardware profile identifier for the match's SIRO wheel polarity. */
#define RC_PROFILE_KURO 1 /* Hardware profile identifier for the match's KURO wheel polarity. */
#define RC_HARDWARE_PROFILE RC_PROFILE_KURO /* First RC robot is KURO (user, 2026-10-13). */
#define RC_DRIBBLER_PWM_MAX 1000U /* Match dribbler input duty denominator. */
#define RC_DRIBBLER_ACTIVE_MAX 990U /* Match avoidance of 100 percent driver duty. */
#define RC_DRIBBLER_POWER_LIMIT 999 /* Match signed dribbler-power clamp. */
#define RC_DRIBBLER_DEFAULT_POWER 990.0 /* Match forward dribbler power when requested. */
#define RC_KICK_HOLD_COUNTS 300U /* Original TIM7-count kick pulse: approximately 150 ms. */
#define RC_KICK_INTERVAL_COUNTS 2000U /* Original cooldown after pulse/cancel: approximately 1000 ms. */
#define RC_LINE_NORMAL_MIN 1.0f /* Below this vector magnitude, retain the previous normal. */
#define RC_LINE_HOLD_MS 150U /* Retain the field normal after circular-line detection clears. */
#define RC_LINE_CLEAR_MS 150U /* Clear period required before a new ring episode can establish a normal. */
#define RC_VECTOR_EPSILON 0.0001f /* Numerical tolerance for line hemisphere/feasibility comparisons. */
#define RC_SWITCH_DEBOUNCE_MS 20U /* Stable switch level required before edge processing. */
#define RC_GREEN_SHORT_MS 1000U /* Releases shorter than one second reset heading while disabled. */
#define RC_GREEN_HOLD_MS 3000U /* Disabled green hold requests pairing once per press. */
#define RC_PAIRING_WINDOW_MS 30000U /* Actuators remain stopped throughout the enrollment window. */
#define RC_BEEP_ENABLE_MS 100U /* Non-blocking motor-enable feedback pulse. */
#define RC_BEEP_RESET_MS 100U /* Non-blocking heading-reset feedback pulse. */
#define RC_BEEP_PAIR_MS 300U /* Non-blocking pairing-window feedback pulse. */

#if RC_HARDWARE_PROFILE != RC_PROFILE_SIRO && RC_HARDWARE_PROFILE != RC_PROFILE_KURO
#error Invalid RC_HARDWARE_PROFILE
#endif

#endif
