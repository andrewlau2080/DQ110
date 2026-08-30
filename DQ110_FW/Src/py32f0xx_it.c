/**
  ******************************************************************************
  * @file    py32f0xx_it.c
  * @brief   中断服务
  *          SysTick：HAL tick
  *          TIM1_BRK_UP_TRG_COM：更新中断 → 32 位捕获时间基准扩展
  *          TIM1_CC：捕获中断 → HAL 分发 → 测速回调
  *          EXTI2_3 / EXTI4_15：档位变化事件（Stop 唤醒）
  ******************************************************************************
  */
#include "main.h"
#include "py32f0xx_it.h"
#include "dq110_tim.h"
#include "dq110_gpio.h"

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
  while (1) {
  }
}

void SVC_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

/**
  * @brief  TIM1 更新中断：CNT 每回绕一次（=DQ110_PWM_MAX tick，50µs），
  *         累加 32 位时间基准，供捕获测速扩展
  */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
  if (__HAL_TIM_GET_FLAG(&DQ110_TimHandle, TIM_FLAG_UPDATE)) {
    __HAL_TIM_CLEAR_FLAG(&DQ110_TimHandle, TIM_FLAG_UPDATE);
    DQ110_CapTick32 += DQ110_PWM_MAX;
  }
}

/**
  * @brief  TIM1 捕获/比较中断 → HAL 分发（触发 HAL_TIM_IC_CaptureCallback）
  */
void TIM1_CC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&DQ110_TimHandle);
}

/**
  * @brief  EXTI 线 2/3（TSSOP20 SW1=PA2；SOP8 SW1=PA3、SW2=PB3）
  */
void EXTI2_3_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2 | GPIO_PIN_3);
}

/**
  * @brief  EXTI 线 4~15（TSSOP20 SW2=PA5）
  */
void EXTI4_15_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
}

/**
  * @brief  EXTI 回调：档位按键事件
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint16_t sw_lines = DQ110_EXTI_SW_LINES;
#ifdef DQ110_EXTI_SW2_LINES
  sw_lines |= DQ110_EXTI_SW2_LINES;
#endif
  if (GPIO_Pin & sw_lines) {
    DQ110_GPIO_NotifyEvent();
  }
}

/************************ (C) COPYRIGHT Puya *****END OF FILE****/
