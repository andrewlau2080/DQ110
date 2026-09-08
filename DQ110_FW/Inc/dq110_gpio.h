/**
  ******************************************************************************
  * @file    dq110_gpio.h
  * @brief   档位检测 GPIO（SW1=PB0 / SW2=PA2）
  ******************************************************************************
  */
#ifndef DQ110_GPIO_H
#define DQ110_GPIO_H

#include "dq110.h"

void    DQ110_GPIO_Init(void);
uint8_t DQ110_Gear_IsActive(DQ110_Gear_t g);   /* 1 = 该档选中（引脚低） */
uint8_t DQ110_Gear_Scan(void);                 /* 主循环轮询，10ms 节拍，3 次防抖 */

#endif /* DQ110_GPIO_H */
