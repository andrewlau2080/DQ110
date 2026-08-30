/**
  ******************************************************************************
  * @file    py32f0xx_it.h
  * @brief   中断服务声明
  ******************************************************************************
  */
#ifndef __PY32F0XX_IT_H
#define __PY32F0XX_IT_H

#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void EXTI2_3_IRQHandler(void);
void EXTI4_15_IRQHandler(void);
void TIM1_BRK_UP_TRG_COM_IRQHandler(void);
void TIM1_CC_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __PY32F0XX_IT_H */
