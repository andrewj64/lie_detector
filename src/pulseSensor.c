#include "pulseSensor.h"

// Variables internal to the pulse detection algorithm.
// Not volatile because we use them only internally to the pulse detection.
unsigned long sampleIntervalMs;  // expected time between calls to readSensor(), in milliseconds.
int rate[10];                    // array to hold last ten IBI values (ms)
unsigned long sampleCounter;     // used to determine pulse timing. Milliseconds since we started.
int P;                           // used to find peak in pulse wave, seeded (sample value)
int T;                           // used to find trough in pulse wave, seeded (sample value)
int thresh;                      // used to find instant moment of heart beat, seeded (sample value)
bool firstBeat;               // used to seed rate array so we startup with reasonable BPM
bool secondBeat;              // used to seed rate array so we startup with reasonable BPM
uint32_t pulse;


int BPM;                // int that holds raw Analog in 0. updated every call to readSensor()
//volatile int Signal;             // holds the latest incoming raw data (0..1023)
int IBI;                // int that holds the time interval (ms) between beats! Must be seeded!
bool Pulse;          // "True" when User's live heartbeat is detected. "False" when not a "live beat".
bool QS;             // The start of beat has been detected and not read by the Sketch.
int threshSetting;      // used to seed and reset the thresh variable
int amp;                         // used to hold amplitude of pulse waveform, seeded (sample value)
unsigned long lastBeatTime;      // used to find IBI. Time (sampleCounter) of the previous detected beat start.

void adc2Init()
{
	// ADC clock and GPIO init
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
	
	//Disable ADC2 by clearing bit ADC_CR_ADEN in register ADC2->CR
	ADC2->CR &= ~ADC_CR_ADEN;
	
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
	ADC2_Wakeup();
	
	// configure RES bits to set resolution as 12 bits
	ADC2->CFGR &= ~ADC_CFGR_RES;
	
	// select right alignment in CFGR
	ADC2->CFGR &= ~ADC_CFGR_ALIGN;
	
	// Select 1 conversion in the regular channel conversion sequence
	ADC2->SQR1 &= ~ADC_SQR1_L;
	
	// Specify the channel number 6 as 1st conversion in regular sequence
	ADC2->SQR1 &= ~ADC_SQR1_SQ1;
	ADC2->SQR1 |= ADC_SQR1_SQ1_1 | ADC_SQR1_SQ1_2 | ADC_SQR1_SQ1_0;	// channel 7

	// configure the channel 6 as single-ended
	ADC2->DIFSEL &= ~ADC_DIFSEL_DIFSEL_7;
	
	// Select ADC sample time (111 = 640.5 ADC clock cycles)
	ADC2->SMPR1 |= ADC_SMPR1_SMP6;
	
	// Select ADC as discontinuous mode
	ADC2->CFGR &= ~ADC_CFGR_CONT;
	
	//Select software trigger
	ADC2->CFGR &= ~ADC_CFGR_EXTEN;
	
//	// select TIM2_TRG0 event (1011) as external trigger for regular channels
//	ADC1->CFGR &= ~ADC_CFGR_EXTSEL;		// clear the EXTSEL
//	ADC1->CFGR |= ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_1 | ADC_CFGR_EXTSEL_0;	// do some fancy stuff to set it to TIM2_TRG0
	
	//Enable ADC1
	ADC2->CR |= ADC_CR_ADEN;
	
	// Wait until ADC1 is ready
	while(!(ADC2->ISR & ADC_ISR_ADRDY));
	// enable ADC handler
	ADC2->IER |= ADC_IER_EOCIE;
	NVIC_EnableIRQ(ADC1_2_IRQn);

	// trigger becomes immediately effective once software starts ADC.
	ADC2->CR |= ADC_CR_ADSTART;
	
	
	//added for final project to make sure the adc works with pa1 without the setup in main
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	GPIOA->MODER |= 3U << 4;		// configure PA2 as analog mode

	// GPIO Push-Pull: No pull-up pull-down (00),
	// Pull-up (01), Pull-down (10), Reserved (11)
	GPIOA->PUPDR &= ~(3U << 4);		// no pull-up pull-down
	
	//GPIOA port analog switch control register (ASCR)
	GPIOA->ASCR |= 1U<<2;
}

void ADC2_Wakeup(void)
{
	if((ADC2->CR & ADC_CR_DEEPPWD) == ADC_CR_DEEPPWD)		// wake up!!
	{
		ADC2->CR &= ~ADC_CR_DEEPPWD;	// exit deep power down mode
	}
	
	ADC2->CR |= ADC_CR_ADVREGEN;	// enable the ADC internal voltage regulator
	
	// wait for the ADC voltage regulator startup time
	int waitTime = 20 * (80000000 / 1000000);
	while(waitTime)
		waitTime--;
}


void resetVariables()
{
	for (int i = 0; i < 10; ++i) {
    rate[i] = 0;
  }
  QS = false;
  BPM = 0;
  IBI = 750;                  // 750ms per beat = 80 Beats Per Minute (BPM)
  Pulse = false;
  sampleCounter = 0;
  lastBeatTime = 0;
  P = 2000;		//about halfway                    // peak at 1/2 the input range of 0..1023
  T = 2000;		//512;                    // trough at 1/2 the input range.
  threshSetting = 2050;//550;        // used to seed and reset the thresh variable
  thresh = 2050;//550;     // threshold a little above the trough
  amp = 400;                  // beat amplitude 1/10 of input range.
  firstBeat = true;           // looking for the first beat
  secondBeat = false;         // not yet looking for the second beat in a row
}

void processLatestSample(uint32_t Signal) {
  // Serial.println(threshSetting);
  // Serial.print('\t');
  // Serial.println(thresh);
  sampleCounter += 1;         			// keep track of the time in mS with this variable
  int N = sampleCounter - lastBeatTime;      // monitor the time since the last beat to avoid noise


  //  find the peak and trough of the pulse wave
  if (Signal < thresh && N > (IBI / 5) * 3) { // avoid dichrotic noise by waiting 3/5 of last IBI
    if (Signal < T) {                        // T is the trough
      T = Signal;                            // keep track of lowest point in pulse wave
    }
  }

  if (Signal > thresh && Signal > P) {       // thresh condition helps avoid noise
    P = Signal;                              // P is the peak
  }                                          // keep track of highest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (N > 250) {                             // avoid high frequency noise
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3) ) {
      Pulse = true;                          // set the Pulse flag when we think there is a pulse
      IBI = sampleCounter - lastBeatTime;    // measure time between beats in mS
      lastBeatTime = sampleCounter;          // keep track of time for next pulse

      if (secondBeat) {                      // if this is the second beat, if secondBeat == TRUE
        secondBeat = false;                  // clear secondBeat flag
        for (int i = 0; i <= 9; i++) {       // seed the running total to get a realisitic BPM at startup
          rate[i] = IBI;
        }
      }

      if (firstBeat) {                       // if it's the first time we found a beat, if firstBeat == TRUE
        firstBeat = false;                   // clear firstBeat flag
        secondBeat = true;                   // set the second beat flag
        // IBI value is unreliable so discard it
        return;
      }


      // keep a running total of the last 10 IBI values
      int runningTotal = 0;                  // clear the runningTotal variable

      for (int i = 0; i <= 8; i++) {          // shift data in the rate array
        rate[i] = rate[i + 1];                // and drop the oldest IBI value
        runningTotal += rate[i];              // add up the 9 oldest IBI values
      }

      rate[9] = IBI;                          // add the latest IBI to the rate array
      runningTotal += rate[9];                // add the latest IBI to runningTotal
      runningTotal /= 10;                     // average the last 10 IBI values
      BPM = 30000 / runningTotal ;             // how many beats can fit into a half minute? that's BPM!
      QS = true;                              // set Quantified Self flag (we detected a beat)
			
			GPIOE->ODR ^= GPIO_ODR_OD8;			// Toggle heartbeat LED
    }
  }

  if (Signal < thresh && Pulse == true) {  // when the values are going down, the beat is over
    Pulse = false;                         // reset the Pulse flag so we can do it again
    amp = P - T;                           // get amplitude of the pulse wave
    thresh = amp / 2 + T;                  // set thresh at 50% of the amplitude
    P = thresh;                            // reset these for next time
    T = thresh;
  }

  if (N > 2500) {                          // if 2.5 seconds go by without a beat
    thresh = threshSetting;                // set thresh default
    P = 2000;                               // set P default
    T = 2000;                               // set T default
    lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date
    firstBeat = true;                      // set these to avoid noise
    secondBeat = false;                    // when we get the heartbeat back
    QS = false;
    BPM = 0;
    IBI = 600;                  // 600ms per beat = 100 Beats Per Minute (BPM)
    Pulse = false;
    amp = 400;                  // beat amplitude 1/10 of input range.

  }
}

uint32_t getBPM()
{
	return BPM;
}
