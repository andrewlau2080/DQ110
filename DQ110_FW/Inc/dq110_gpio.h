#ifndef DQ110_GPIO_H
#define DQ110_GPIO_H
#include "dq110.h"
void    DQ110_GPIO_Init(void);
uint8_t DQ110_Gear_Scan(void);
uint8_t DQ110_VbusPresent(void);
void    DQ110_MT_Enable(uint8_t enable);
void    DQ110_Buck_Enable(uint8_t enable);
#endif
