#include "main.h"
#include "dq110_power.h"
#include "dq110_gpio.h"
#include "dq110_adc.h"
#include "dq110_tim.h"

volatile DQ110_Source_t g_dq110_source = DQ110_SOURCE_NONE;
volatile uint8_t g_dq110_fault = 0U;
static uint8_t s_restart_required;
static uint8_t s_restart_armed;

static void DQ110_Source_Stop(void)
{
  DQ110_MT_Enable(0U);
  DQ110_Buck_Enable(0U);
  DQ110_TIM_SetDuty(0U);
  DQ110_TIM_PWM_Stop();
}

static uint8_t DQ110_BatteryAvailable(void)
{
  return DQ110_ADC_GetBatteryMv() >= DQ110_BAT_RESTART_MV;
}

static void DQ110_SelectUsb(void)
{
  /* USB path is already present through D_USB5. Keep the buck alive. */
  DQ110_MT_Enable(0U);
  DQ110_Buck_Enable(1U);
  g_dq110_source = DQ110_SOURCE_USB;
}

static void DQ110_SelectBattery(void)
{
  /* Start boost first, then keep/enable the common buck. */
  DQ110_MT_Enable(1U);
  DQ110_Buck_Enable(1U);
  g_dq110_source = DQ110_SOURCE_BAT;
  s_restart_required = 0U;
  s_restart_armed = 0U;
}

void DQ110_PWR_Init(void)
{
  s_restart_required = 0U;
  s_restart_armed = 0U;
  DQ110_Source_Stop();
  HAL_Delay(5U);
  if (DQ110_VbusPresent()) {
    DQ110_SelectUsb();
  } else if (DQ110_BatteryAvailable()) {
    DQ110_SelectBattery();
  } else {
    g_dq110_source = DQ110_SOURCE_BAT_UNAVAILABLE;
    s_restart_required = 1U;
    s_restart_armed = 1U;
  }
}

void DQ110_PWR_Tick(uint32_t now_ms)
{
  static uint32_t last_source_check = 0U;
  static uint32_t last_bat_check = 0U;
  static uint8_t vbus_count = 0U;
  uint8_t vbus;
  uint16_t bat_mv;

  if ((now_ms - last_source_check) < 10U) return;
  last_source_check = now_ms;
  vbus = DQ110_VbusPresent();

  if (vbus) {
    if (vbus_count < 2U) vbus_count++;
  } else {
    vbus_count = 0U;
  }

  /* USB has priority whenever it is detected. */
  if (vbus_count >= 2U && g_dq110_source != DQ110_SOURCE_USB) {
    DQ110_SelectUsb();
  }

  /* USB removal is a deliberate stop. Do not silently restart from the
   * battery; the next 0 -> gear transition must re-arm the source check. */
  if (vbus_count == 0U && g_dq110_source == DQ110_SOURCE_USB) {
    g_dq110_source = DQ110_SOURCE_BAT_UNAVAILABLE;
    s_restart_required = 1U;
    s_restart_armed = (g_dq110_gear == DQ110_GEAR_OFF);
    DQ110_Source_Stop();
  }

  if (g_dq110_source == DQ110_SOURCE_BAT &&
      (now_ms - last_bat_check) >= 100U) {
    last_bat_check = now_ms;
    bat_mv = DQ110_ADC_GetBatteryMv();
    if (bat_mv <= DQ110_BAT_LOW_MV) {
      /* 260916: low-voltage stop is latched until a restart is requested. */
      g_dq110_source = DQ110_SOURCE_BAT_UNAVAILABLE;
      s_restart_required = 1U;
      s_restart_armed = (g_dq110_gear == DQ110_GEAR_OFF);
      DQ110_Source_Stop();
    }
  }

  /* A restart is armed by the 0 gear. Battery restart is allowed only after
   * the battery has recovered above DQ110_BAT_RESTART_MV. */
  if (g_dq110_source == DQ110_SOURCE_BAT_UNAVAILABLE) {
    if (g_dq110_gear == DQ110_GEAR_OFF) {
      s_restart_armed = 1U;
    }
    if (vbus_count >= 2U) {
      DQ110_SelectUsb();
    } else if (DQ110_BatteryAvailable() &&
               (!s_restart_required || s_restart_armed)) {
      DQ110_SelectBattery();
    }
  }
}

uint8_t DQ110_PWR_RunAllowed(void)
{
  return (g_dq110_source == DQ110_SOURCE_BAT ||
          g_dq110_source == DQ110_SOURCE_USB) && !g_dq110_fault;
}
