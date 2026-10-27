#include "main.h"
#include "dq110_gpio.h"

static DQ110_Gear_t s_last = DQ110_GEAR_OFF;
static uint8_t s_db = 0;

void DQ110_GPIO_Init(void)
{
  GPIO_InitTypeDef g = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* EN pins are open-drain: external 1M pull-up provides the high level. */
  g.Pin = DQ110_MT_EN_PIN | DQ110_BUCK_EN_PIN;
  g.Mode = GPIO_MODE_OUTPUT_OD;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &g);
  HAL_GPIO_WritePin(GPIOA, DQ110_MT_EN_PIN | DQ110_BUCK_EN_PIN, GPIO_PIN_RESET);

  g.Pin = DQ110_GEAR1_PIN | DQ110_GEAR2_PIN;
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &g);

  g.Pin = DQ110_VBUS_DET_PIN;
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DQ110_VBUS_DET_PORT, &g);

  g.Pin = DQ110_PWM_PIN;
  g.Mode = GPIO_MODE_OUTPUT_PP;
  g.Pull = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &g);
  HAL_GPIO_WritePin(GPIOA, DQ110_PWM_PIN, GPIO_PIN_RESET);

  g.Pin = DQ110_BAT_ADC_PIN | DQ110_CUR_ADC_PIN;
  g.Mode = GPIO_MODE_ANALOG;
  g.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &g);
}

uint8_t DQ110_VbusPresent(void)
{
  return HAL_GPIO_ReadPin(DQ110_VBUS_DET_PORT, DQ110_VBUS_DET_PIN) == GPIO_PIN_SET;
}

void DQ110_MT_Enable(uint8_t enable)
{
  HAL_GPIO_WritePin(DQ110_MT_EN_PORT, DQ110_MT_EN_PIN,
                   enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void DQ110_Buck_Enable(uint8_t enable)
{
  HAL_GPIO_WritePin(DQ110_BUCK_EN_PORT, DQ110_BUCK_EN_PIN,
                   enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t DQ110_Gear_Scan(void)
{
  DQ110_Gear_t raw = DQ110_GEAR_OFF;
  if (HAL_GPIO_ReadPin(DQ110_GEAR2_PORT, DQ110_GEAR2_PIN) == GPIO_PIN_RESET) {
    raw = DQ110_GEAR2;
  } else if (HAL_GPIO_ReadPin(DQ110_GEAR1_PORT, DQ110_GEAR1_PIN) == GPIO_PIN_RESET) {
    raw = DQ110_GEAR1;
  }
  if (raw == s_last) {
    if (s_db < 3U) s_db++;
  } else {
    s_last = raw;
    s_db = 1U;
  }
  if (s_db >= 3U && g_dq110_gear != s_last) {
    g_dq110_gear = s_last;
    return 1U;
  }
  return 0U;
}
