#include "DAC.h"

#define M_PI 3.141592653589793

int sine_table2[41] = {0x800, 0x823, 0x847, 0x86b, 0x88e, 0x8b2, 0x8d6, 0x8f9, 0x91d, 0x940, 0x963, 0x986, 0x9a9, 0x9cc, 0x9ef, 0xa12, 0xa34, 0xa56, 0xa78, 0xa9a, 0xabc, 0xadd, 0xaff, 0xb20, 0xb40, 0xb61, 0xb81, 0xba1, 0xbc1, 0xbe0, 0xc00, 0xc1e, 0xc3d, 0xc5b, 0xc79, 0xc96, 0xcb3, 0xcd0, 0xcec, 0xd08};
int sine_table[41]; 
int entry = 1;

void dacInit(void)
{
	//enable dac clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_DAC1EN;
	
	//disable dac
	DAC->CR &= ~(DAC_CR_EN1 | DAC_CR_EN2);
	
	//set mode
	DAC->MCR	&=~(7U<<16);
	
	//enable trigger for dac 2
	DAC->CR	|= DAC_CR_TEN2;
	
	//select software trigger
	DAC->CR |= DAC_CR_TSEL2;
	
	// enable dac channel 2
	DAC->CR |= DAC_CR_EN2; 
	
	//enable clock of GPIO A
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	
	GPIOA->MODER |= 3U<<(10);
}

void createSineTable(void)
{
	float sf;
	for (int i = 0; i < 41; i++)
	{
		sf = sin(M_PI * 9 * i/180);
		sine_table[i*9] = (1 + sf) * 2048;
		if(sine_table[i*9] == 0x1000)
			sine_table[i*9] = 0xFFF; 		// sin(90) is bad juju
	}
}

uint32_t lookupSine(int x)
{
	// x is the input in degrees
	
	x = (x*9) % 360;
	if(x < 90) return sine_table[x];
	if(x < 180) return sine_table[180-x];
	if(x < 270) return 4096 - sine_table[x-180];
	return 4096 - sine_table[360-x];
}

// configure timer C
void TIM4_Init(void)
{
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM4EN;	// enable clock of timer 2
//	TIM4->CR1 &= ~TIM_CR1_CMS;			// clear edge-aligned mode bits
	TIM4->CR1 &= ~TIM_CR1_DIR;		// counting direction = up
	
//	TIM4->CR2 &= ~TIM_CR2_MMS;			// clear master mode selection bits
//	TIM4->CR2 |= TIM_CR2_MMS_2;			// select 100 = OC1REF as TRGO
	
	//OC1M: Output compare 1 mode
//	TIM4->CCMR1 &= ~TIM_CCMR1_OC1M;
//	TIM4->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;		// 0110 = PWM mode 1
	
	// timer driving frequency = 16 MHz/(1+PSC) = 16 MHz/(1+7) = 2 MHz
	// trigger frequency = 2 MHz/(1+ARR) = 2MHz/(1+3999) = 500 Hz
	//TIM4->PSC  = 7;
	
//	TIM4->ARR  = 19999;
//	TIM4->CCR1 = 10000;		// duty ratio 50%
	
	TIM4->PSC  = 7;
	TIM4->ARR  = 1999;
	TIM4->CCR1 = 1000;		// duty ratio 50%
//	TIM4->PSC  = 1599;
//	TIM4->ARR  = 99;
//	TIM4->CCR1 = 50;		// duty ratio 50%
	TIM4->DIER |= TIM_DIER_UIE;
	
//	TIM4->CCER |= TIM_CCER_CC1E;	//OC1 signal is output
	NVIC_EnableIRQ(TIM4_IRQn);
	TIM4->CR1  |= TIM_CR1_CEN;		// enable timer dangit
}

void TIM4_IRQHandler(void)
{
	//update to the next val in sine table
	// wait until DAC is not busy
	// set DAC output
	//while(DAC->SR & DAC_SR_BWST2);	// wait until DAC isn't busy
	//DAC->DHR12R2 = sine_table2[entry%41];
	DAC->DHR12R2 = lookupSine(entry);
	
	// Start software trigger
	DAC->SWTRIGR |= DAC_SWTRIGR_SWTRIG2;
	
	// increment output
	entry++;
	//TIM4->SR &= ~TIM_SR_UIF; // clear flag
}
