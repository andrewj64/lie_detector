#include "stm32l476xx.h"
#include <stdint.h>

void ADC1_Wakeup(void);
void adcInit(void);
void TIM2_Init(void);
void TIM2_IRQHandler(void);
uint32_t getResult(void);
