#include "resistivity.h"

void rInit()
{
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;	//enable port clocks
	
	// configure PE10
	GPIOE->MODER |=  (3<<10);			// set to analog
}

