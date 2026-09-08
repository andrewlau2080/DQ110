/**
  ******************************************************************************
  * @file    dq110_tim.h
  * @brief   软件 PWM（TIM14 160kHz 中断帧, 8kHz; 半级占空比单位 0~40）
  ******************************************************************************
  */
#ifndef DQ110_TIM_H
#define DQ110_TIM_H

#include "dq110.h"

extern TIM_HandleTypeDef DQ110_TimHandle;

void     DQ110_TIM_Init(void);
void     DQ110_TIM_PWM_Start(void);
void     DQ110_TIM_PWM_Stop(void);
void     DQ110_TIM_SetDuty(uint16_t duty);   /* 0 ~ DQ110_PWM_MAX（半级单位 0~40） */
uint16_t DQ110_TIM_GetDuty(void);
void     DQ110_TIM_SoftPwmTick(void);        /* TIM14 更新中断内调用（20kHz） */

#endif /* DQ110_TIM_H */
