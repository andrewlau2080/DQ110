#ifndef DQ110_POWER_H
#define DQ110_POWER_H
#include "dq110.h"
void    DQ110_PWR_Init(void);
void    DQ110_PWR_Tick(uint32_t now_ms);
uint8_t DQ110_PWR_RunAllowed(void);
#endif
