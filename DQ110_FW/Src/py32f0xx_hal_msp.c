/**
  ******************************************************************************
  * @file    py32f0xx_hal_msp.c
  * @brief   HAL MSP 初始化：外设时钟 / GPIO 复用 / 中断优先级
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
  * @brief  TIM1 MSP：PWM 引脚（PA4 AF13）+ 捕获引脚（PB6 AF1）+ CC 中断
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef gi = {0};

  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();   /* PWM GPIO（两封装均在 GPIOA） */
  __HAL_RCC_GPIOB_CLK_ENABLE();   /* 捕获 GPIO：TSSOP20 PB6；SOP8 的 PB3(SW2) 在 gpio.c 已使能 */

  /* M- 驱动 PWM 引脚 */
  gi.Pin       = DQ110_PWM_GPIO_PIN;
  gi.Mode      = GPIO_MODE_AF_PP;
  gi.Pull      = GPIO_NOPULL;
  gi.Speed     = GPIO_SPEED_FREQ_HIGH;
  gi.Alternate = DQ110_PWM_AF;
  HAL_GPIO_Init(DQ110_PWM_GPIO_PORT, &gi);

  /* HALL 捕获引脚（外部 4.7k 上拉，内部不加上拉）——仅霍尔闭环版 */
#ifndef DQ110_NO_HALL
  gi.Pin       = DQ110_CAP_GPIO_PIN;
  gi.Mode      = GPIO_MODE_AF_PP;
  gi.Pull      = GPIO_NOPULL;
  gi.Speed     = GPIO_SPEED_FREQ_HIGH;
  gi.Alternate = DQ110_CAP_AF;
  HAL_GPIO_Init(DQ110_CAP_GPIO_PORT, &gi);

  /* 捕获中断优先级 0（最高，保证测速精度）；更新中断 1 在 DQ110_TIM_Init 配置 */
  HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
#endif
}

/**
  * @brief  ADC MSP：过流检测引脚模拟输入
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  GPIO_InitTypeDef gi = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  gi.Pin  = DQ110_OC_GPIO_PIN;
  gi.Mode = GPIO_MODE_ANALOG;
  gi.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DQ110_OC_GPIO_PORT, &gi);
}

/************************ (C) COPYRIGHT Puya *****END OF FILE****/
