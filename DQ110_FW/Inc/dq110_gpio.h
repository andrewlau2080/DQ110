/**
  ******************************************************************************
  * @file    dq110_gpio.h
  * @brief   档位检测 GPIO/EXTI（SW1/SW2）
  ******************************************************************************
  */
#ifndef DQ110_GPIO_H
#define DQ110_GPIO_H

#include "main.h"

void DQ110_GPIO_Init(void);
uint8_t DQ110_Gear_Scan(void);    /* 10ms 周期调用：防抖确认后更新 g_dq110_gear；返回 1=档位有更新 */
uint8_t DQ110_Gear_IsActive(DQ110_Gear_t g); /* 硬件引脚即时读（单次） */
void DQ110_GPIO_NotifyEvent(void); /* EXTI 回调调用：置档位事件 */
uint8_t DQ110_GPIO_HasEvent(void); /* 查询未消费事件 */

#endif /* DQ110_GPIO_H */
