/**
  ******************************************************************************
  * @file    dq110_adc.h
  * @brief   过流检测 ADC（采样电阻 + 运放 → 12 位 ADC）
  ******************************************************************************
  */
#ifndef DQ110_ADC_H
#define DQ110_ADC_H

#include "main.h"

extern ADC_HandleTypeDef DQ110_AdcHandle;

void DQ110_ADC_Init(void);
uint16_t DQ110_ADC_ReadRaw(void);       /* 单次软件触发转换（当前配置通道） */
uint16_t DQ110_ADC_ReadFiltered(void);  /* 过流通道 8 点滑动平均 */
uint16_t DQ110_ADC_ReadVrefintRaw(void);/* 采样 VREFINT 通道（低成本版反推电池电压用） */
uint32_t DQ110_OC_ThresholdLsb(uint16_t current_ma); /* 电流阈值 → ADC 原始值 */

#endif /* DQ110_ADC_H */
