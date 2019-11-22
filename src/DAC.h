#include "stm32l476xx.h"
#include <stdint.h>
#include <math.h>

void dacInit(void);
void createSineTable(void);
uint32_t lookupSine(int x);
void TIM4_Init(void);
void TIM4_IRQHandler(void);
