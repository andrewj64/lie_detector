#include "ADC.h"

uint32_t resistivity, beats;

void adcInit(void)
{
	// ADC clock and GPIO init
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
	
	//Disable ADC1 by clearing bit ADC_CR_ADEN in register ADC1->CR
	ADC1->CR &= ~ADC_CR_ADEN;
	
	//Enable I/O analog switches voltage booster
	SYSCFG->CFGR1 |= SYSCFG_CFGR1_BOOSTEN;

	// Enable the conversion of internal channels
	ADC123_COMMON->CCR |= ADC_CCR_VREFEN;
	
	// do some stuff
	RCC->CFGR &= ~RCC_CFGR_HPRE_3;
	
	//configure the ADC prescaler to 0
	ADC123_COMMON->CCR &= ~ADC_CCR_PRESC;
	
	// select synchronous clock mode(HCLK/1 = 01)
	ADC123_COMMON->CCR &= ~ADC_CCR_CKMODE;
	ADC123_COMMON->CCR |= ADC_CCR_CKMODE_0;
	
	// configure all ADCs as independent
	ADC123_COMMON->CCR &= ~ADC_CCR_DUAL;
	
	//turn off deep power down mode
	ADC1_Wakeup();
	
	// configure RES bits to set resolution as 12 bits
	ADC1->CFGR &= ~ADC_CFGR_RES;
	
	// select right alignment in CFGR
	ADC1->CFGR &= ~ADC_CFGR_ALIGN;
	
	// Select 1 conversion in the regular channel conversion sequence
	ADC1->SQR1 &= ~ADC_SQR1_L;
	
	// Specify the channel number 6 as 1st conversion in regular sequence
	ADC1->SQR1 &= ~ADC_SQR1_SQ1;
	ADC1->SQR1 |= ADC_SQR1_SQ1_1 | ADC_SQR1_SQ1_2;	// channel 6

	// configure the channel 6 as single-ended
	ADC1->DIFSEL &= ~ADC_DIFSEL_DIFSEL_6;
	
	// Select ADC sample time (111 = 640.5 ADC clock cycles)
	ADC1->SMPR1 |= ADC_SMPR1_SMP6;
	
	// Select ADC as discontinuous mode
	ADC1->CFGR &= ~ADC_CFGR_CONT;
	
	//Select software trigger
	ADC1->CFGR &= ~ADC_CFGR_EXTEN;
	
//	// select TIM2_TRG0 event (1011) as external trigger for regular channels
//	ADC1->CFGR &= ~ADC_CFGR_EXTSEL;		// clear the EXTSEL
//	ADC1->CFGR |= ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_1 | ADC_CFGR_EXTSEL_0;	// do some fancy stuff to set it to TIM2_TRG0
	
	//Enable ADC1
	ADC1->CR |= ADC_CR_ADEN;
	
	// Wait until ADC1 is ready
	while(!(ADC1->ISR & ADC_ISR_ADRDY));
	// enable ADC handler
	ADC1->IER |= ADC_IER_EOCIE;
	NVIC_EnableIRQ(ADC1_2_IRQn);

	// trigger becomes immediately effective once software starts ADC.
	ADC1->CR |= ADC_CR_ADSTART;
	
	
	//added for final project to make sure the adc works with pa1 without the setup in main
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	GPIOA->MODER |= 3U << 2;		// configure PA1 as analog mode
		
	// GPIO Push-Pull: No pull-up pull-down (00),
	// Pull-up (01), Pull-down (10), Reserved (11)
	GPIOA->PUPDR &= ~(3U << 2);		// no pull-up pull-down
	
	//GPIOA port analog switch control register (ASCR)
	GPIOA->ASCR |= 1U<<1;	
}

// configure timer A
void TIM2_Init(void)
{
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;	// enable clock of timer 2
	//TIM2->CR1 &= ~TIM_CR1_CMS;			// clear edge-aligned mode bits
	TIM2->CR1 &= ~TIM_CR1_DIR;		// counting direction = up
	
//	TIM2->CR2 &= ~TIM_CR2_MMS;			// clear master mode selection bits
//	TIM2->CR2 |= TIM_CR2_MMS_2;			// select 100 = OC1REF as TRGO
	
//	//OC1M: Output compare 1 mode
//	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M;
//	TIM2->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;		// 0110 = PWM mode 1
	
	// timer driving frequency = 2 MHz/(1+PSC) = 2 MHz/(1+7) = 2 MHz
	// trigger frequency = 2 MHz/(1+ARR) = 2MHz/(1+399) = 5000 Hz
	TIM2->PSC  = 7;
	TIM2->ARR  = 399;
	TIM2->CCR1 = 200;		// duty ratio 50%
	TIM2->DIER |= TIM_DIER_UIE;
	
	//TIM2->CCER |= TIM_CCER_CC1E;	//OC1 signal is output
	NVIC_EnableIRQ(TIM2_IRQn);

	TIM2->CR1  |= TIM_CR1_CEN;		// enable timer dangit
	

}

void TIM3_Init(void)
{
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;	// enable clock of timer 2
	TIM3->CR1 &= ~TIM_CR1_DIR;		// counting direction = up
	
	// timer driving frequency = 2 MHz/(1+PSC) = 2 MHz/(1+7) = 2 MHz
	// trigger frequency = 2 MHz/(1+ARR) = 2MHz/(1+3999) = 500 Hz
	TIM3->PSC  = 7;
	TIM3->ARR  = 1999;
	TIM3->CCR1 = 1000;		// duty ratio 50%
	TIM3->DIER |= TIM_DIER_UIE;
	
	NVIC_EnableIRQ(TIM3_IRQn);

	TIM3->CR1  |= TIM_CR1_CEN;		// enable timer dangit
	

}

void ADC1_Wakeup(void)
{
	if((ADC1->CR & ADC_CR_DEEPPWD) == ADC_CR_DEEPPWD)		// wake up!!
	{
		ADC1->CR &= ~ADC_CR_DEEPPWD;	// exit deep power down mode
	}
	
	ADC1->CR |= ADC_CR_ADVREGEN;	// enable the ADC internal voltage regulator
	
	// wait for the ADC voltage regulator startup time
	int waitTime = 20 * (80000000 / 1000000);
	while(waitTime)
		waitTime--;
}


void TIM3_IRQHandler()
{
	processLatestSample(beats);
	TIM3->SR &= ~TIM_SR_UIF; // clear flag
}

void TIM2_IRQHandler()
{
	//start the conversion in software
	//if((ADC1->ISR & ADC_ISR_EOC) != ADC_ISR_EOC)
	ADC1->CR |= ADC_CR_ADSTART;

	//for (int i = 0; i < 600; i++);
	TIM2->SR &= ~TIM_SR_UIF; // clear flag
}

void ADC1_2_IRQHandler()
{
	if((ADC1->ISR & ADC_ISR_EOC) == ADC_ISR_EOC)
	{
		resistivity = ADC1->DR;
		ADC2->CR |= ADC_CR_ADSTART;
	} else if((ADC2->ISR & ADC_ISR_EOC) == ADC_ISR_EOC)
	{
		beats = ADC2->DR;
		//ADC1->CR |= ADC_CR_ADSTART;

	}
}

uint32_t getResistivity()
{
	return resistivity;
}

