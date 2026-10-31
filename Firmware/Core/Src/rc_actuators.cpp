#include "rc_internal.hpp"
#include "chassis.hpp"
#include <cmath>

namespace {
Chassis<Omni_4> omni;
uint32_t period_1 = 1800U, period_8 = 1800U, period_3 = 370U;
double dribbler_power = 0.0;
uint16_t cnt = 0, kicking_start_time = 0, kick_interval_start_time = 0;
bool kick_interval_active = false;
volatile bool& kicking_active = rc_state.kick_active;
uint32_t beep_started = 0, beep_duration = 0;
bool beep_active = false;
constexpr uint16_t DRIBBLER_PWM_MAX = RC_DRIBBLER_PWM_MAX;
constexpr uint16_t DRIBBLER_ACTIVE_MAX = RC_DRIBBLER_ACTIVE_MAX;
constexpr int16_t DRIBBLER_POWER_LIMIT = RC_DRIBBLER_POWER_LIMIT;
constexpr uint16_t KICK_HOLD_COUNT = RC_KICK_HOLD_COUNTS;
constexpr uint16_t KICK_INTERVAL_COUNT = RC_KICK_INTERVAL_COUNTS;

// Copied match dribbler helpers; timer-period scaling and reversal gap retained.
uint32_t dribbler_compare(TIM_HandleTypeDef *htim, uint16_t duty)
{
  if (duty > DRIBBLER_PWM_MAX) duty = DRIBBLER_PWM_MAX;
  if (duty == DRIBBLER_PWM_MAX) duty = DRIBBLER_ACTIVE_MAX;
  const uint32_t period = __HAL_TIM_GET_AUTORELOAD(htim) + 1U;
  return ((period * duty) + (DRIBBLER_PWM_MAX / 2U)) / DRIBBLER_PWM_MAX;
}

void set_dribbler_pwm(uint16_t drb1_duty, uint16_t drb2_duty)
{
  static bool initialized = false;
  static uint16_t last_drb1_duty = 0, last_drb2_duty = 0;
  if (drb1_duty > DRIBBLER_PWM_MAX) drb1_duty = DRIBBLER_PWM_MAX;
  if (drb2_duty > DRIBBLER_PWM_MAX) drb2_duty = DRIBBLER_PWM_MAX;
  const bool reverse_change = initialized &&
      ((last_drb1_duty > 0 && drb2_duty > 0) || (last_drb2_duty > 0 && drb1_duty > 0));
  if (reverse_change) {
    __HAL_TIM_SET_COMPARE(&htim13, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);
    for (volatile uint32_t i = 0; i < 1000U; ++i) __NOP();
  }
  __HAL_TIM_SET_COMPARE(&htim13, TIM_CHANNEL_1, dribbler_compare(&htim13, drb1_duty));
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, dribbler_compare(&htim14, drb2_duty));
  last_drb1_duty = drb1_duty; last_drb2_duty = drb2_duty; initialized = true;
}

double clamp_dribbler_power(double power)
{
  if (power > DRIBBLER_POWER_LIMIT) return DRIBBLER_POWER_LIMIT;
  if (power < -DRIBBLER_POWER_LIMIT) return -DRIBBLER_POWER_LIMIT;
  return power;
}
uint16_t dribbler_duty_from_power(double power)
{
  const double magnitude = power < 0.0 ? -power : power;
  return static_cast<uint16_t>((magnitude * DRIBBLER_ACTIVE_MAX / DRIBBLER_POWER_LIMIT) + 0.5);
}
void apply_dribbler_power(bool force_stop)
{
  if (force_stop) { set_dribbler_pwm(0, 0); return; }
  dribbler_power = clamp_dribbler_power(dribbler_power);
  const uint16_t duty = dribbler_duty_from_power(dribbler_power);
  if (duty == 0) set_dribbler_pwm(0, 0);
  else if (dribbler_power > 0.0) set_dribbler_pwm(0, duty);
  else set_dribbler_pwm(duty, 0);
}

// Copied match count-based pulse/cooldown logic. IRQ caller supplies fresh cnt.
bool kick_interval_is_active()
{
  if (!kick_interval_active) return false;
  if (static_cast<uint16_t>(cnt-kick_interval_start_time) > KICK_INTERVAL_COUNT) {
    kick_interval_active = false; return false;
  }
  return true;
}
bool update_kicking_hysteresis()
{
  if (kicking_active) {
    HAL_GPIO_WritePin(KICK2_GPIO_Port, KICK2_Pin, GPIO_PIN_SET);
    if (static_cast<uint16_t>(cnt-kicking_start_time) > KICK_HOLD_COUNT) {
      kicking_active = false; kick_interval_active = true; kick_interval_start_time = cnt;
      HAL_GPIO_WritePin(KICK2_GPIO_Port, KICK2_Pin, GPIO_PIN_RESET);
    }
    return true;
  }
  kick_interval_is_active();
  HAL_GPIO_WritePin(KICK2_GPIO_Port, KICK2_Pin, GPIO_PIN_RESET);
  return false;
}

void wheel_compares(int32_t a, int32_t b, int32_t c, int32_t d) {
    // Omni order FL, BL, BR, FR; original SIRO/KURO polarity retained.
    __HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_3,static_cast<uint32_t>(static_cast<int32_t>(period_1/2U)-a));
    const int32_t bl_sign = RC_HARDWARE_PROFILE == RC_PROFILE_SIRO ? 1 : -1;
    __HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_2,static_cast<uint32_t>(static_cast<int32_t>(period_1/2U)+bl_sign*b));
    __HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,static_cast<uint32_t>(static_cast<int32_t>(period_1/2U)+c));
    __HAL_TIM_SET_COMPARE(&htim8,TIM_CHANNEL_3,static_cast<uint32_t>(static_cast<int32_t>(period_8/2U)+d));
}
}

void rc_actuators_cutoff() {
    // Only registers: callable from SysTick and fatal handlers without HAL waits.
    if ((RCC->APB2ENR & RCC_APB2ENR_TIM1EN) != 0U) {
        TIM1->BDTR &= ~TIM_BDTR_MOE;
        TIM1->CCER &= ~(TIM_CCER_CC1E|TIM_CCER_CC1NE|TIM_CCER_CC2E|TIM_CCER_CC2NE|TIM_CCER_CC3E|TIM_CCER_CC3NE);
    }
    if ((RCC->APB2ENR & RCC_APB2ENR_TIM8EN) != 0U) {
        TIM8->BDTR &= ~TIM_BDTR_MOE;
        TIM8->CCER &= ~(TIM_CCER_CC3E|TIM_CCER_CC3NE);
    }
    if ((RCC->APB1ENR & RCC_APB1ENR_TIM13EN) != 0U) TIM13->CCR1 = 0U;
    if ((RCC->APB1ENR & RCC_APB1ENR_TIM14EN) != 0U) TIM14->CCR1 = 0U;
    if ((RCC->AHB1ENR & RCC_AHB1ENR_GPIOAEN) != 0U) {
        GPIOA->BSRR = static_cast<uint32_t>(KICK2_Pin)<<16U;
        GPIOA->BSRR = SW2_LED_Pin;
    }
    if (kicking_active) {
        kicking_active = false; kick_interval_active = true;
        kick_interval_start_time = static_cast<uint16_t>(TIM7->CNT);
    }
    rc_state.wheels_active = false;
    rc_state.wheels_parked = false;
    rc_state.dribbler_active = false;
}

void rc_actuators_service_stop() {
    // Reconcile HAL channel state after an interrupt performed a register cutoff.
    (void)HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_1);
    (void)HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_1);
    (void)HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_2);
    (void)HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_2);
    (void)HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_3);
    (void)HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_3);
    (void)HAL_TIM_PWM_Stop(&htim8,TIM_CHANNEL_3);
    (void)HAL_TIMEx_PWMN_Stop(&htim8,TIM_CHANNEL_3);
    apply_dribbler_power(true);
    wheel_compares(0,0,0,0);
}

bool rc_actuators_init() {
    rc_actuators_cutoff();
    period_1 = __HAL_TIM_GET_AUTORELOAD(&htim1)+1U;
    period_8 = __HAL_TIM_GET_AUTORELOAD(&htim8)+1U;
    period_3 = __HAL_TIM_GET_AUTORELOAD(&htim3)+1U;
    if (period_1/2U <= RC_WHEEL_COMPARE_MARGIN || period_8/2U <= RC_WHEEL_COMPARE_MARGIN) return false;
    omni.set_limit(static_cast<double>((period_1 < period_8 ? period_1 : period_8)/2U-RC_WHEEL_COMPARE_MARGIN));
    rc_actuators_service_stop();
    if (HAL_TIM_PWM_Start(&htim13,TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim14,TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_4) != HAL_OK) return false;
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_4,0U);
    HAL_GPIO_WritePin(KICK1_GPIO_Port,KICK1_Pin,GPIO_PIN_SET); // Original match idle level.
    return true;
}

// Dribbler and enable LED while driving is permitted (wheels running or parked).
static void apply_permitted_outputs(bool dribbler) {
    const bool run_dribbler = dribbler && !rc_state.dribbler_blocked;
    dribbler_power = run_dribbler ? RC_DRIBBLER_DEFAULT_POWER : 0.0;
    apply_dribbler_power(!run_dribbler);
    rc_state.dribbler_active = run_dribbler;
    HAL_GPIO_WritePin(SW2_LED_GPIO_Port,SW2_LED_Pin,GPIO_PIN_RESET);
}

void rc_actuators_park(bool dribbler) {
    RcCritical lock;
    if (!rc_permission(HAL_GetTick())) { rc_stop(rc_gate_fault(HAL_GetTick())); return; }
    if (rc_state.wheels_active) {
        // Same output-off path as a normal stop, but without blocking the dribbler or requiring rearm.
        (void)HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_1);
        (void)HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_1);
        (void)HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_2);
        (void)HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_2);
        (void)HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_3);
        (void)HAL_TIMEx_PWMN_Stop(&htim1,TIM_CHANNEL_3);
        (void)HAL_TIM_PWM_Stop(&htim8,TIM_CHANNEL_3);
        (void)HAL_TIMEx_PWMN_Stop(&htim8,TIM_CHANNEL_3);
        wheel_compares(0,0,0,0);
        rc_state.wheels_active = false;
    }
    rc_state.wheels_parked = true;
    apply_permitted_outputs(dribbler);
}

void rc_actuators_apply(RcVector body, float correction, bool dribbler) {
    const float power = std::sqrt(body.x*body.x+body.y*body.y)*RC_MATCH_TRANSLATION_FULL/1000.0f;
    const float mv_deg = std::atan2(body.x,body.y)*(180.0f/3.14159265358979323846f);
    omni.dcalc(90.0-static_cast<double>(mv_deg),static_cast<double>(power),static_cast<double>(correction));
    int32_t wheels[4];
    for (unsigned i=0; i<4; ++i) wheels[i] = static_cast<int32_t>(std::lround(*omni.get_motor(static_cast<int>(i))));
    RcCritical lock;
    if (!rc_permission(HAL_GetTick())) { rc_stop(rc_gate_fault(HAL_GetTick())); return; }
    if (!rc_state.wheels_active) {
        wheel_compares(0,0,0,0); // Never enable stale compare values.
        bool ok = HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1) == HAL_OK;
        ok = (HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_1) == HAL_OK) && ok;
        ok = (HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2) == HAL_OK) && ok;
        ok = (HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_2) == HAL_OK) && ok;
        ok = (HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3) == HAL_OK) && ok;
        ok = (HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_3) == HAL_OK) && ok;
        ok = (HAL_TIM_PWM_Start(&htim8,TIM_CHANNEL_3) == HAL_OK) && ok;
        ok = (HAL_TIMEx_PWMN_Start(&htim8,TIM_CHANNEL_3) == HAL_OK) && ok;
        if (!ok) { rc_stop(RC_FAULT_INITIALIZATION,true); rc_state.ready = false; return; }
        rc_state.wheels_active = true;
    }
    wheel_compares(wheels[0],wheels[1],wheels[2],wheels[3]);
    rc_state.wheels_parked = false;
    apply_permitted_outputs(dribbler);
}

void rc_kick_request() {
    RcCritical lock;
    if (!rc_permission(HAL_GetTick())) return; // Interlock applies at the actual output action.
    cnt = static_cast<uint16_t>(TIM7->CNT);
    if (kicking_active || kick_interval_is_active()) return;
    kicking_start_time = cnt; kicking_active = true;
    HAL_GPIO_WritePin(KICK2_GPIO_Port,KICK2_Pin,GPIO_PIN_SET);
}
void rc_kick_tick() {
    RcCritical lock;
    cnt = static_cast<uint16_t>(TIM7->CNT);
    (void)update_kicking_hysteresis();
}
void rc_beep(uint32_t duration_ms) {
    beep_started = HAL_GetTick(); beep_duration = duration_ms; beep_active = true;
    __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_4,period_3/2U);
}
void rc_beep_poll(uint32_t now) {
    if (beep_active && now-beep_started >= beep_duration) {
        beep_active = false; __HAL_TIM_SET_COMPARE(&htim3,TIM_CHANNEL_4,0U);
    }
}
