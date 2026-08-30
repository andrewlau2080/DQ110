/**
  ******************************************************************************
  * @file    dq110_tim.h
  * @brief   TIM1：PWM 输出（M- 驱动）+ 输入捕获（HALL 测速）
  ******************************************************************************
  */
#ifndef DQ110_TIM_H
#define DQ110_TIM_H

#include "main.h"

extern TIM_HandleTypeDef DQ110_TimHandle;   /* it.c 使用 */
extern volatile uint32_t DQ110_CapTick32;   /* 更新中断扩展的 32 位捕获时间基准 */

void DQ110_TIM_Init(void);
void DQ110_TIM_PWM_Start(void);
void DQ110_TIM_PWM_Stop(void);
void DQ110_TIM_SetDuty(uint16_t duty);      /* 0 ~ DQ110_PWM_MAX */
uint16_t DQ110_TIM_GetDuty(void);
void DQ110_TIM_CapStart(void);
uint32_t DQ110_TIM_GetLastCapTick(void);    /* 最近捕获的 32 位 tick（堵转检测用） */

#endif /* DQ110_TIM_H */
