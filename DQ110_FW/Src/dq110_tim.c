/**
  ******************************************************************************
  * @file    dq110_tim.c
  * @brief   软件 PWM 马达控制
  *
  *         硬件：002B SOP8 的 PB6（pin5，马达 PWM 脚）无 TIM1 通道，
  *               用 TIM14 更新中断产生 20kHz 时基，中断内比较翻转 PB6。
  *         时基：TIM14 PSC=0 ARR=1199 → 20kHz（50µs）更新中断
  *         中断负载：每 50µs 一次简单比较+写引脚，24MHz 下开销可忽略
  ******************************************************************************
  */
#include "main.h"
#include "dq110_tim.h"

TIM_HandleTypeDef DQ110_TimHandle;

static volatile uint16_t s_duty   = 0;    /* 目标占空比 0~40（半级单位） */
static volatile uint16_t s_pwmcnt = 0;    /* 帧内计数 0~19（ISR 内） */
static volatile uint8_t  s_parity = 0;    /* 半级抖动帧标志（ISR 内） */

/**
  * @brief  TIM14 初始化并启动（配置定时器；实际启停由档位逻辑控制）
  */
void DQ110_TIM_Init(void)
{
  DQ110_TimHandle.Instance               = TIM14;
  DQ110_TimHandle.Init.Prescaler         = 0;
  DQ110_TimHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
  DQ110_TimHandle.Init.Period            = DQ110_PWM_PERIOD;      /* 149 → 160kHz ISR */
  DQ110_TimHandle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  DQ110_TimHandle.Init.RepetitionCounter = 0;
  DQ110_TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&DQ110_TimHandle) != HAL_OK) {
    APP_ErrorHandler();
  }
  /* 注意：不在初始化时启动，由 ApplyGear 拨档后 Start（0 档停中断省电） */
}

/**
  * @brief  启动软件 PWM（TIM14 更新中断）
  */
void DQ110_TIM_PWM_Start(void)
{
  if (HAL_TIM_Base_Start_IT(&DQ110_TimHandle) != HAL_OK) {
    APP_ErrorHandler();
  }
}

/**
  * @brief  停止软件 PWM（马达停，输出拉低）
  */
void DQ110_TIM_PWM_Stop(void)
{
  HAL_TIM_Base_Stop_IT(&DQ110_TimHandle);
  HAL_GPIO_WritePin(DQ110_PWM_OUT_PORT, DQ110_PWM_OUT_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  设置占空比（0~DQ110_PWM_MAX）
  */
void DQ110_TIM_SetDuty(uint16_t duty)
{
  if (duty > DQ110_PWM_MAX) duty = DQ110_PWM_MAX;
  s_duty = duty;
}

uint16_t DQ110_TIM_GetDuty(void)
{
  return s_duty;
}

/**
  * @brief  软件 PWM 相位推进（TIM14 中断内调用，寄存器直写，160kHz 下必须最短）
  *         帧 = 20 中断(8kHz)；占空比单位 = 半级(0~40)：
  *         奇数单位 = 隔帧多输出 1 级（半级抖动 → 平均 2.5% 精度）
  */
void DQ110_TIM_SoftPwmTick(void)
{
  uint16_t cnt = s_pwmcnt + 1U;
  uint16_t eff;

  if (cnt >= DQ110_PWM_RES) {
    cnt = 0;
  }
  s_pwmcnt = cnt;
  if (cnt == 0U) {
    s_parity ^= 1U;                    /* 帧起始翻转 → 帧间交替 */
  }
  eff = s_duty >> 1U;                  /* 整级部分 */
  if ((s_duty & 1U) && s_parity) {
    eff++;                             /* 半级：隔帧 +1 级 */
  }
  if (eff > DQ110_PWM_RES) {
    eff = DQ110_PWM_RES;               /* 上限 20 */
  }
  if (cnt < eff) {
    GPIOB->BSRR = (uint32_t)DQ110_PWM_OUT_PIN;           /* PB6 置位 */
  } else {
    GPIOB->BSRR = (uint32_t)DQ110_PWM_OUT_PIN << 16U;    /* PB6 复位 */
  }
}

/************************ (C) COPYRIGHT Puya *****END OF FILE****/
