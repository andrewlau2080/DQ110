/**
  ******************************************************************************
  * @file    dq110_power.h
  * @brief   低功耗：0 档 Stop 模式（待机 ≤5µA）+ 状态机转移
  ******************************************************************************
  */
#ifndef DQ110_POWER_H
#define DQ110_POWER_H

#include "main.h"

void DQ110_PWR_Init(void);       /* Stop 模式参数配置 */
void DQ110_Power_Manage(void);   /* 主循环调用：状态机转移 + Stop 进入/唤醒恢复 */

#endif /* DQ110_POWER_H */
