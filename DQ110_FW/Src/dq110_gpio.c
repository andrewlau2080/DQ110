/**
  ******************************************************************************
  * @file    dq110_gpio.c
  * @brief   档位检测 GPIO/EXTI（SW1/SW2）
  *          规格书：SW 内部上拉、低有效；三档船型开关公共端接 BAT-
  ******************************************************************************
  */
#include "main.h"
#include "dq110_gpio.h"

/* EXTI 事件标志：中断里置位，唤醒/主循环立即扫描一次 */
static volatile uint8_t s_gear_event = 0;

/* 防抖状态 */
static DQ110_Gear_t s_last_gear    = DQ110_GEAR_OFF;
static uint8_t      s_debounce_cnt = 0;

/**
  * @brief  初始化 SW1/SW2：内部上拉 + 下降沿 EXTI（唤醒 Stop）
  */
void DQ110_GPIO_Init(void)
{
  GPIO_InitTypeDef gi = {0};

  /* SW1 */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  gi.Pin   = DQ110_SW1_GPIO_PIN;
  gi.Mode  = GPIO_MODE_IT_FALLING;
  gi.Pull  = GPIO_PULLUP;
  gi.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DQ110_SW1_GPIO_PORT, &gi);

  /* SW2 */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  gi.Pin   = DQ110_SW2_GPIO_PIN;
  gi.Mode  = GPIO_MODE_IT_FALLING;
  gi.Pull  = GPIO_PULLUP;
  gi.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DQ110_SW2_GPIO_PORT, &gi);

  /* TSSOP20 未用脚 PF0/PF1：输出低防悬空漏电（规格书 7.4） */
#ifndef DQ110_PKG_SOP8
  __HAL_RCC_GPIOF_CLK_ENABLE();
  gi.Pin   = GPIO_PIN_0 | GPIO_PIN_1;
  gi.Mode  = GPIO_MODE_OUTPUT_PP;
  gi.Pull  = GPIO_NOPULL;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &gi);
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);
#else
  /* SOP8：PA14 复位默认 SWCLK，运行释放为 GPIO，输入下拉防漏电（pin4 用 PB3 功能） */
  gi.Pin   = GPIO_PIN_14;
  gi.Mode  = GPIO_MODE_INPUT;
  gi.Pull  = GPIO_PULLDOWN;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &gi);
#endif

#ifdef DQ110_NO_HALL
  /* 低成本版（无霍尔）：HALL 引脚释放，输入下拉防悬空漏电
   * （SOP8=PA4 / TSSOP20=PB6，原霍尔输入脚） */
  gi.Pin   = DQ110_CAP_GPIO_PIN;
  gi.Mode  = GPIO_MODE_INPUT;
  gi.Pull  = GPIO_PULLDOWN;
  gi.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DQ110_CAP_GPIO_PORT, &gi);
#endif

  /* EXTI 中断使能（SW1；SOP8 下 SW2 也在此） */
  HAL_NVIC_SetPriority(DQ110_EXTI_SW_IRQ, 2, 0);
  HAL_NVIC_EnableIRQ(DQ110_EXTI_SW_IRQ);

#ifdef DQ110_EXTI_SW2_IRQ   /* TSSOP20：SW2=PA5 → EXTI4_15 */
  HAL_NVIC_SetPriority(DQ110_EXTI_SW2_IRQ, 2, 0);
  HAL_NVIC_EnableIRQ(DQ110_EXTI_SW2_IRQ);
#endif
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
    s_gear_event = 0;   /* 消费 EXTI 事件 */
    confirmed = s_last_gear;
    if (g_dq110_gear != confirmed) {
      g_dq110_gear = confirmed;
      return 1;
    }
  }
  return 0;
}

/**
  * @brief  EXTI 中断置位事件（由 HAL_GPIO_EXTI_Callback 调用）
  */
void DQ110_GPIO_NotifyEvent(void)
{
  s_gear_event = 1;
}

/**
  * @brief  查询是否有未消费的档位事件（唤醒后立即扫描用）
  */
uint8_t DQ110_GPIO_HasEvent(void)
{
  return s_gear_event;
}
