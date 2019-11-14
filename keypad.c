#include "keypad.h"

unsigned char key_map[4][4] = {
	{'D','#','0','*'},
	{'C','9','8','7'},
	{'B','6','5','4'},
	{'A','3','2','1'}
};

void keypad_pin_init(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOEEN;
	
	GPIOA->MODER &= 0xFFFFF303U;		//set 1,2,3,5 to input   
	
	GPIOE->MODER &= 0xF00FFFFFU;		//set E 10,11,12,13 as output
	GPIOE->MODER |= 0x05500000U;
	
	GPIOE->OTYPER |= 0x3C00U;				// set E 10-13 to open-drain
}

/*
* BEGIN
*		Check for any press
*		Debounce counter loop
*		Loop through rows
*			set row to 0
*			Loop
*				Check if input after a short delay (not all ones)
*					store key pressed
*			EndLoop when at last column
*			set row to 1
*		EndLoop when key is pressed
*		Wait until no input (all ones)
*		return key from keymap[row][col]
* END
*/
unsigned char keypad_scan(void)
{
	unsigned char row, col, shift;
	unsigned char key = 0;
	
	// Check whether any key has been pressed
	// 	Output zeros on all row pins
	//	Delay shortly, and read inputs of column pins
	//	If inputs are 1 for all columns, then no key has been pressed
	
	while(!key)
	{
		GPIOE->ODR |= 0x3C00U;						// Set all rows to 1
		for(int i = 0; i < 2000; i++);		// Short debounce maybe get rid of this?
		
		for(row = 0; row < 4; row++)
		{
			GPIOE->ODR &= ~(1<<(10+row));		// Set row to 0
			for(col = 0; col < 4; col++)
			{
				for(int i = 0; i < 1000; i++);	// short delay after setting row to 0
				shift = col < 3 ? col + 1 : col + 2;	// 1,2,3,5
				if(~GPIOA->IDR & (1U << shift))				// check if col is pressed
				{
					key = key_map[col][row];
					while(~GPIOA->IDR & (1U << shift));	// wait until key is released
				}				
			}
			GPIOE->ODR |= (1<<(10+row));	// set row back to 1
		}
	}
	
	return key;
	
}
