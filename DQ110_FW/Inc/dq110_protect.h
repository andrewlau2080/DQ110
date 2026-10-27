/**
  ******************************************************************************
  * @file    dq110_protect.h
  * @brief   保护：过流分级保护 + 堵转检测 + IWDG 看门狗
  ******************************************************************************
  */
#ifndef DQ110_PROTECT_H
#define DQ110_PROTECT_H

#include "main.h"

extern IWDG_HandleTypeDef DQ110_IwdgHandle;

void DQ110_IWDG_Init(void);            /* LSI 使能 + IWDG（预分频64/重载4095 → 8s 超时） */
void DQ110_Protect_Run(uint32_t now_ms); /* 主循环 10ms 调用：过流采样 + 堵转检测 */
void DQ110_Protect_StartRun(uint32_t now_ms); /* 进入 RUN 时记录启动时刻（堵转宽限） */

#endif /* DQ110_PROTECT_H */
