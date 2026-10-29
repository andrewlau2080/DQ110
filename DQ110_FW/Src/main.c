/** DQ110 firmware V2.1: 260916 customer-confirmed dual-gear control. */
#include "main.h"
#include "dq110_gpio.h"
#include "dq110_adc.h"
#include "dq110_tim.h"
#include "dq110_power.h"

volatile DQ110_Gear_t g_dq110_gear = DQ110_GEAR_OFF;
volatile uint16_t DQ110_AD_LIVE;

static uint16_t s_cur_duty;
static uint16_t s_tgt_duty;
static uint32_t s_run_start_ms;
static uint8_t s_oc_cnt;
static uint8_t s_last_power_ok;
static uint32_t s_last_speed_update_ms;
static uint8_t s_motor_started;
static uint8_t s_soft_start;

static uint16_t DQ110_TargetForGear(void)
{
  uint32_t duty;
  uint16_t bat_mv;

  if (g_dq110_gear == DQ110_GEAR_OFF) return 0U;
  duty = (g_dq110_gear == DQ110_GEAR2) ?
         DQ110_GEAR2_DUTY : DQ110_GEAR1_DUTY;
  bat_mv = DQ110_ADC_GetBatteryMv();

  /* 260916 requires speed control only above 2.5V. Use voltage
   * feed-forward below the 3.0V calibration point; below 2.5V the
   * motor is allowed to slow down without a speed guarantee. */
  if (bat_mv >= DQ110_SPEED_COMP_MIN_MV && bat_mv < DQ110_SPEED_CAL_MV) {
    duty = (duty * DQ110_SPEED_CAL_MV + (bat_mv / 2U)) / bat_mv;
  }
  if (duty > DQ110_PWM_MAX) duty = DQ110_PWM_MAX;
  return (uint16_t)duty;
}

static void DQ110_SystemClock_Config(void)
{
  RCC_OscInitTypeDef ro = {0};
  RCC_ClkInitTypeDef rc = {0};
  ro.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  ro.HSIState = RCC_HSI_ON;
  ro.HSICalibrationValue = RCC_HSICALIBRATION_24MHz;
  if (HAL_RCC_OscConfig(&ro) != HAL_OK) APP_ErrorHandler();
  rc.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  rc.SYSCLKSource = RCC_SYSCLKSOURCE_HSISYS;
  rc.AHBCLKDivider = RCC_SYSCLK_DIV1;
  rc.APB1CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&rc, FLASH_LATENCY_0) != HAL_OK) APP_ErrorHandler();
}

static void DQ110_StopMotor(void)
{
  s_cur_duty = 0U;
  s_tgt_duty = 0U;
  s_oc_cnt = 0U;
  s_motor_started = 0U;
  s_soft_start = 0U;
  DQ110_TIM_SetDuty(0U);
  DQ110_TIM_PWM_Stop();
}

static void DQ110_ApplyGear(void)
{
  if (g_dq110_gear == DQ110_GEAR_OFF) {
    /* A 0 -> gear transition is the requested restart after a latched
     * low-voltage or over-current stop. The power module still requires
     * the battery to be at least DQ110_BAT_RESTART_MV. */
    g_dq110_fault = 0U;
    DQ110_StopMotor();
    return;
  }
  if (!DQ110_PWR_RunAllowed()) {
    DQ110_StopMotor();
    return;
  }
  s_tgt_duty = DQ110_TargetForGear();
  s_oc_cnt = 0U;
  if (s_cur_duty == 0U) s_soft_start = 1U;
}

static void DQ110_SpeedControlTick(uint32_t now_ms)
{
  if ((now_ms - s_last_speed_update_ms) < 100U) return;
  s_last_speed_update_ms = now_ms;
  if (DQ110_PWR_RunAllowed() && g_dq110_gear != DQ110_GEAR_OFF) {
    s_tgt_duty = DQ110_TargetForGear();
  }
}

static void DQ110_RampTick(uint32_t now_ms)
{
  if (!DQ110_PWR_RunAllowed() || g_dq110_gear == DQ110_GEAR_OFF) {
    DQ110_StopMotor();
    return;
  }

  if (!s_motor_started) {
    if (!DQ110_PWR_MotorReady(now_ms)) {
      DQ110_TIM_SetDuty(0U);
      DQ110_TIM_PWM_Stop();
      return;
    }
    DQ110_TIM_SetDuty(0U);
    DQ110_TIM_PWM_Start();
    s_motor_started = 1U;
    s_run_start_ms = now_ms;
  }

  {
    uint16_t step = s_soft_start ? DQ110_START_RAMP_STEP : DQ110_RAMP_STEP;
  if (s_cur_duty < s_tgt_duty) {
    s_cur_duty += step;
    if (s_cur_duty > s_tgt_duty) s_cur_duty = s_tgt_duty;
  } else if (s_cur_duty > s_tgt_duty) {
    s_cur_duty -= step;
    if (s_cur_duty < s_tgt_duty) s_cur_duty = s_tgt_duty;
  }
  if (s_cur_duty == s_tgt_duty) s_soft_start = 0U;
  }
  DQ110_TIM_SetDuty(s_cur_duty);
}

static void DQ110_OverCurrentTick(uint32_t now_ms)
{
  uint16_t cur_mv;
  uint16_t lim;
  if (!DQ110_PWR_RunAllowed() || g_dq110_gear == DQ110_GEAR_OFF) return;
  if (!s_motor_started) return;
  if ((now_ms - s_run_start_ms) < DQ110_OC_BLANK_MS) return;
  cur_mv = DQ110_ADC_GetCurrentMv();
  DQ110_AD_LIVE = cur_mv;
  lim = (g_dq110_gear == DQ110_GEAR2) ? DQ110_CUR_TRIP_G2_MV : DQ110_CUR_TRIP_G1_MV;
  if (cur_mv >= lim) {
    if (++s_oc_cnt >= DQ110_OC_CONFIRM_CNT) {
      g_dq110_fault = 1U;
      DQ110_StopMotor();
    }
  } else {
    s_oc_cnt = 0U;
  }
}

int main(void)
{
  uint32_t now;
  HAL_Init();
  DQ110_SystemClock_Config();
  DQ110_GPIO_Init();
  DQ110_ADC_Init();
  DQ110_TIM_Init();
  DQ110_PWR_Init();

  while (1) {
    now = HAL_GetTick();
    DQ110_PWR_Tick(now);
    if (!s_last_power_ok && DQ110_PWR_RunAllowed() &&
        g_dq110_gear != DQ110_GEAR_OFF) {
      DQ110_ApplyGear();
    }
    s_last_power_ok = DQ110_PWR_RunAllowed();
    if (DQ110_Gear_Scan()) {
      DQ110_ApplyGear();
      /* ApplyGear() records a fresh run-start timestamp. Refresh now so
       * the over-current blanking interval cannot underflow on a gear edge. */
      now = HAL_GetTick();
    }
    DQ110_SpeedControlTick(now);
    DQ110_OverCurrentTick(now);
    DQ110_RampTick(now);
    HAL_Delay(10U);
  }
}

void APP_ErrorHandler(void)
{
  while (1) { }
}
