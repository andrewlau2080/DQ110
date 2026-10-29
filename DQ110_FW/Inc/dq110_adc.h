#ifndef DQ110_ADC_H
#define DQ110_ADC_H
#include "dq110.h"
void     DQ110_ADC_Init(void);
uint16_t DQ110_ADC_GetVccMv(void);
uint16_t DQ110_ADC_GetBatteryMv(void);
uint16_t DQ110_ADC_GetVbusDetMv(void);
uint16_t DQ110_ADC_GetCurrentMv(void);
#endif
