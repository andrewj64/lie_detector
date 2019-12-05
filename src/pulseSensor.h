#ifndef PULSE_SENSOR_H
#define PULSE_SENSOR_H

#include <stdbool.h>
#include <stdio.h>
#include "stm32l476xx.h"

// Pulse detection output variables.
// Volatile because our pulse detection code could be called from an Interrupt




void adc2Init(void);
void ADC2_Wakeup(void);
void resetVariables(void);
void processLatestSample(uint32_t Signal);
uint32_t getBPM(void);



#endif
