/**
  ******************************************************************************
  * @file    py32f0xx_hal_msp.c
  * @brief   HAL MSP 初始化：外设时钟 / 中断
  ******************************************************************************
  */
#include "main.h"

/**
  * @brief  全局 MSP 初始化
  */
void HAL_MspInit(void)
{
}

/**
  * @brief  TIM14 MSP：时钟使能 + 更新中断使能（软件 PWM 时基）
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM14) {
    __HAL_RCC_TIM14_CLK_ENABLE();

    /* 软件 PWM 中断：优先级 1（SysTick 0 优先不受影响） */
    HAL_NVIC_SetPriority(TIM14_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM14_IRQn);
  }
}

/**
  * @brief  ADC MSP：时钟（校准已在 dq110_adc.c 内完成）
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  __HAL_RCC_ADC_CLK_ENABLE();
}

/************************ (C) COPYRIGHT Puya *****END OF FILE****/
