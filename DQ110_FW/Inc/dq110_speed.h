/**
  ******************************************************************************
  * @file    dq110_speed.h
  * @brief   转速闭环：PID 稳速（一档 15RPM / 二档 30RPM）
  ******************************************************************************
  */
#ifndef DQ110_SPEED_H
#define DQ110_SPEED_H

#include "main.h"

void DQ110_Speed_Control(uint32_t now_ms); /* 主循环调用，内部按 DQ110_CTRL_PERIOD_MS 节拍 */
void DQ110_Speed_Reset(void);              /* 状态切换/换挡时清零 PID 状态 */

#endif /* DQ110_SPEED_H */
