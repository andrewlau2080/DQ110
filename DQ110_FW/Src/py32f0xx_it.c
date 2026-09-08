/**
  ******************************************************************************
  * @file    py32f0xx_it.c
  * @brief   中断服务（002B 精简版）
  *          SysTick：HAL tick
  *          TIM14：20kHz 软件 PWM 时基
  ******************************************************************************
  */
#include "main.h"
#include "py32f0xx_it.h"
#include "dq110_tim.h"

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
  * @brief  TIM14 更新中断 → 软件 PWM 相位推进
  *         寄存器直写（不经 HAL 分发，保证 160kHz 下 ISR 最短，主循环不被饿死）
  */
void TIM14_IRQHandler(void)
{
  if ((TIM14->SR & TIM_SR_UIF) != 0U) {
    TIM14->SR = (uint16_t)~(uint16_t)TIM_SR_UIF;   /* 清更新标志 */
    DQ110_TIM_SoftPwmTick();
  }
}

/************************ (C) COPYRIGHT Puya *****END OF FILE****/
