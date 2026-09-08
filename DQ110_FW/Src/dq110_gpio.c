/**
  ******************************************************************************
  * @file    dq110_gpio.c
  * @brief   引脚初始化 + 档位检测（PY32F002B SOP8，板上实测）
  *           SW1=PB0 / SW2=PA2：内部上拉、低有效，主循环 10ms 防抖轮询
  *           PA2 复位默认 SWCLK、PB6 复位默认 SWDIO——运行时释放为 GPIO，
  *           重新烧录需断电再上电（复位瞬间 SWD 有效）
  ******************************************************************************
  */
#include "main.h"
#include "dq110_gpio.h"

/* 防抖状态 */
static DQ110_Gear_t s_last_gear    = DQ110_GEAR_OFF;
static uint8_t      s_debounce_cnt = 0;

/**
  * @brief  引脚初始化
  */
void DQ110_GPIO_Init(void)
{
  GPIO_InitTypeDef gi = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* PWM 输出 PB6：先普通推挽输出低（软件 PWM 中断接管后按占空比翻转） */
  gi.Pin   = DQ110_PWM_OUT_PIN;
  gi.Mode  = GPIO_MODE_OUTPUT_PP;
  gi.Pull  = GPIO_NOPULL;
  gi.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DQ110_PWM_OUT_PORT, &gi);
  HAL_GPIO_WritePin(DQ110_PWM_OUT_PORT, DQ110_PWM_OUT_PIN, GPIO_PIN_RESET);

  /* SW1 = PB0：输入上拉，低有效 */
  gi.Pin   = DQ110_SW1_GPIO_PIN;
  gi.Mode  = GPIO_MODE_INPUT;
  gi.Pull  = GPIO_PULLUP;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DQ110_SW1_GPIO_PORT, &gi);

  /* SW2 = PA2：释放 SWCLK → GPIO 输入上拉，低有效 */
  gi.Pin   = DQ110_SW2_GPIO_PIN;
  gi.Mode  = GPIO_MODE_INPUT;
  gi.Pull  = GPIO_PULLUP;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DQ110_SW2_GPIO_PORT, &gi);

  /* AD 电流检测 PA7：模拟输入（本次不采样，仅释放为模拟；后续加保护逻辑） */
  gi.Pin   = DQ110_ADC_GPIO_PIN;
  gi.Mode  = GPIO_MODE_ANALOG;
  gi.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(DQ110_ADC_GPIO_PORT, &gi);

  /* pin6 = 复位：固件不配置（硬件复位功能） */

  /* pin2 = PB1 未用：输入下拉防漏电 */
  gi.Pin   = GPIO_PIN_1;
  gi.Mode  = GPIO_MODE_INPUT;
  gi.Pull  = GPIO_PULLDOWN;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &gi);
}

/**
  * @brief  硬件引脚即时读：SWn 低 = 档位选中
  */
uint8_t DQ110_Gear_IsActive(DQ110_Gear_t g)
{
  switch (g) {
  case DQ110_GEAR1:
    return (HAL_GPIO_ReadPin(DQ110_SW1_GPIO_PORT, DQ110_SW1_GPIO_PIN) == GPIO_PIN_RESET);
  case DQ110_GEAR2:
    return (HAL_GPIO_ReadPin(DQ110_SW2_GPIO_PORT, DQ110_SW2_GPIO_PIN) == GPIO_PIN_RESET);
  default:
    return 0;
  }
}

/**
  * @brief  档位扫描（主循环 10ms 调用）：3 次连续一致确认后更新 g_dq110_gear
  * @retval 1 = 档位有更新；0 = 无变化/未确认
  */
uint8_t DQ110_Gear_Scan(void)
{
  DQ110_Gear_t raw;
  DQ110_Gear_t confirmed;

  if (DQ110_Gear_IsActive(DQ110_GEAR1)) {
    raw = DQ110_GEAR1;
  } else if (DQ110_Gear_IsActive(DQ110_GEAR2)) {
    raw = DQ110_GEAR2;
  } else {
    raw = DQ110_GEAR_OFF;
  }

  if (raw == s_last_gear) {
    if (s_debounce_cnt < 3U) s_debounce_cnt++;
  } else {
    s_last_gear    = raw;
    s_debounce_cnt = 1;
  }

  if (s_debounce_cnt >= 3U) {
    confirmed = s_last_gear;
    if (g_dq110_gear != confirmed) {
      g_dq110_gear = confirmed;
      return 1;
    }
  }
  return 0;
}

/************************ (C) COPYRIGHT Puya *****END OF FILE****/
