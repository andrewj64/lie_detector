#ifndef MOTOR_H 
#define MOTOR_H

#include "stm32l476xx.h"
#include <stdint.h>
#include <stdbool.h>

void motor_init(void);
void set_speed(int speed);
void wind_up(int time);
void tick_up(void);
void tick_down(void);

#endif
