#include "stm32l476xx.h"
#include <stdint.h>

#include "pulseSensor.h"

void ADC1_Wakeup(void);
void adcInit(void);
void TIM2_Init(void);
void TIM2_IRQHandler(void);
void TIM3_Init(void);
uint32_t getResistivity(void);

