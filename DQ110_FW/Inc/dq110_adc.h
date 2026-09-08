/**
  ******************************************************************************
  * @file    dq110_adc.h
  * @brief   电池电压检测（ADC 内部 VREFINT 反推 VDD）
  ******************************************************************************
  */
#ifndef DQ110_ADC_H
#define DQ110_ADC_H

#include "dq110.h"

void     DQ110_ADC_Init(void);
uint16_t DQ110_ADC_GetVbatMv(void);      /* 滤波后电池电压 mV（2600~3400 量级） */
uint8_t  DQ110_ADC_Update(void);         /* 主循环 100ms 调用：采 1 次并滤波 */
uint16_t DQ110_ADC_GetCurrentAd(void);   /* 采一次马达电流 AD（PA7 通道4；失败 0xFFFF） */

#endif /* DQ110_ADC_H */
