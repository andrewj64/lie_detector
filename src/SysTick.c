#include "Systick.h"
#include "motor.h"

volatile int sec;
volatile bool count = false;
volatile bool buzz = false;


void SysTick_Init(void) {
SysTick->CTRL = 0; 						// Disable SysTick
	
SysTick->LOAD = 3860320; 			// Set reload register
	
// Set interrupt priority of SysTick to least urgency (i.e., largest priority value)
NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
	
SysTick->VAL = 0; 						// Reset the SysTick counter value
	
// Select processor clock: 1 = processor clock; 0 = external clock
SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
	
// Enables SysTick interrupt, 1 = Enable, 0 = Disable
SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	
// Enable SysTick
SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
	buzz = false;
	if(count)												//if we are supposed to count.....
	{
		sec--;
		tick_down();
		if(sec == 0)										//when we reach 0
		{
			count = false;									//set seconds to 0 
			buzz = true;
		}
	}
}

void setSec(uint32_t countTime)
{
	sec = countTime;
	count = true;
}

void buzzOn(void)
{
	//wait until buzz is true
	while(!buzz);

	//loop while the buzz is true and turn on the buzzer
	while(buzz)
	{
		for(int i = 0; i < 800; i++);	// short delay
		GPIOE->ODR |= 0x100U;					// set voltage to high
		for(int i = 0; i < 800; i++);	// short delay
		GPIOE->ODR &= ~0x100U;					// set voltage to low
		
	}
}

