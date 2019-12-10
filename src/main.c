#include "stm32l476xx.h"
#include "Systick.h"
#include "LCD.h"
#include "motor.h"
#include "ADC.h"
#include "resistivity.h"
#include "pulseSensor.h"
#include <stdlib.h>

//*************************************  32L476GDISCOVERY ***************************************************************************
// STM32L4:  STM32L476VGT6 MCU = ARM Cortex-M4 + FPU + DSP, 
//           LQFP100, 1 MB of Flash, 128 KB of SRAM
//           Instruction cache = 32 lines of 4x64 bits (1KB)
//           Data cache = 8 lines of 4x64 bits (256 B)
//
// Joystick (MT-008A): 
//   Right = PA2        Up   = PA3         Center = PA0
//   Left  = PA1        Down = PA5
//
// User LEDs: 
//   LD4 Red   = PB2    LD5 Green = PE8
//   
// CS43L22 Audio DAC Stereo (I2C address 0x94):  
//   SAI1_MCK = PE2     SAI1_SD  = PE6    I2C1_SDA = PB7    Audio_RST = PE3    
//   SAI1_SCK = PE5     SAI1_FS  = PE4    I2C1_SCL = PB6                                           
//
// MP34DT01 Digital MEMS microphone 
//    Audio_CLK = PE9   Audio_DIN = PE7
//
// LSM303C eCompass (a 3D accelerometer and 3D magnetometer module): 
//   MEMS_SCK  = PD1    MAG_DRDY = PC2    XL_CS  = PE0             
//   MEMS_MOSI = PD4    MAG_CS  = PC0     XL_INT = PE1       
//                      MAG_INT = PC1 
//
// L3GD20 Gyro (three-axis digital output): 
//   MEMS_SCK  = PD1    GYRO_CS   = PD7
//   MEMS_MOSI = PD4    GYRO_INT1 = PD2
//   MEMS_MISO = PD3    GYRO_INT2 = PB8
//
// ST-Link V2 (Virtual com port, Mass Storage, Debug port): 
//   USART_TX = PD5     SWCLK = PA14      MFX_USART3_TX   MCO
//   USART_RX = PD6     SWDIO = PA13      MFX_USART3_RX   NRST
//   PB3 = 3V3_REG_ON   SWO = PB5      
//
// Quad SPI Flash Memory (128 Mbit)
//   QSPI_CS  = PE11    QSPI_D0 = PE12    QSPI_D2 = PE14
//   QSPI_CLK = PE10    QSPI_D1 = PE13    QSPI_D3 = PE15
//
// LCD (24 segments, 4 commons, multiplexed 1/4 duty, 1/3 bias) on DIP28 connector
//   VLCD = PC3
//   COM0 = PA8     COM1  = PA9      COM2  = PA10    COM3  = PB9
//   SEG0 = PA7     SEG6  = PD11     SEG12 = PB5     SEG18 = PD8
//   SEG1 = PC5     SEG7  = PD13     SEG13 = PC8     SEG19 = PB14
//   SEG2 = PB1     SEG8  = PD15     SEG14 = PC6     SEG20 = PB12
//   SEG3 = PB13    SEG9  = PC7      SEG15 = PD14    SEG21 = PB0
//   SEG4 = PB15    SEG10 = PA15     SEG16 = PD12    SEG22 = PC4
//   SEG5 = PD9     SEG11 = PB4      SEG17 = PD10    SEG23 = PA6
// 
// USB OTG
//   OTG_FS_PowerSwitchOn = PC9    OTG_FS_VBUS = PC11    OTG_FS_DM = PA11  
//   OTG_FS_OverCurrent   = PC10   OTG_FS_ID   = PC12    OTG_FS_DP = PA12  
//
// PC14 = OSC32_IN      PC15 = OSC32_OUT
// PH0  = OSC_IN        PH1  = OSC_OUT 
// 
// PA4  = DAC1_OUT1 (NLMFX0 WAKEUP)   PA5 = DAC1_OUT2 (Joy Down)
// PA3  = OPAMP1_VOUT (Joy Up)        PB0 = OPAMP2_VOUT (LCD SEG21)
//
//****************************************************************************************************************
/*
*	BEGIN
		initilaize all our peripherals
		loop until baseline is established? (stops on button press)
			read pulse and resistivity
			output data
			store baseline data to calc if lie?
		endloop
		loop forever
			read in resistivity and pulse data
			output data to LCD
			determine if it is a lie
			if lie,
				output motor to shoot in face
				maybe buzz some ancienct runes if we feel like doing that.
		never endloop
* END
*	
*
*/ 
uint8_t * toString(int x);
void shoot(void);

int main(void){
	int displaySetting = 0;
	int n;
	float baselineBPM, baselineR;
	uint32_t inputR, inputBPM;
	uint32_t threshR_comb = 100;
	uint32_t threshBPM_comb = 9;
	uint32_t threshR = 120;
	uint32_t threshBPM = 15;
	bool truth = true;
	bool check = true;
	
	// Switch system clock to HSI here
 	RCC->CR |= RCC_CR_HSION;
	
	while((RCC->CR & RCC_CR_HSIRDY) == 0); //wait for ready bit
	
	adcInit(); //uses PA1
	adc2Init(); // uses PA2
	LCD_Initialization();		//no gpio
	// pins used by LCD
	//   VLCD = PC3
	//
	//   COM0 = PA8     COM1  = PA9      COM2  = PA10    COM3  = PB9
	//
	//   SEG0 = PA7     SEG6  = PD11     SEG12 = PB5     SEG18 = PD8
	//   SEG1 = PC5     SEG7  = PD13     SEG13 = PC8     SEG19 = PB14
	//   SEG2 = PB1     SEG8  = PD15     SEG14 = PC6     SEG20 = PB12
	//   SEG3 = PB13    SEG9  = PC7      SEG15 = PD14    SEG21 = PB0
	//   SEG4 = PB15    SEG10 = PA15     SEG16 = PD12    SEG22 = PC4
	//   SEG5 = PD9     SEG11 = PB4      SEG17 = PD10    SEG23 = PA6
	TIM2_Init();
	TIM3_Init();
	resetVariables();
	motor_init(); //uses PB2,PB3,PB6,PB7
	set_speed(8000);	//for motor
	
	// Configure green LED for BPM
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
	GPIOE->MODER &= ~GPIO_MODER_MODE8;
	GPIOE->MODER |= GPIO_MODER_MODE8_0;		// configure PE8 to output mode
	
	//initialize pa5 for the button overlay
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	GPIOA->MODER &= ~(0xC03);
	GPIOA->PUPDR &= ~(0xC03);
	GPIOA->PUPDR |= 0x802;
	
	uint8_t* string;
	string = (uint8_t*)"test   ";
	LCD_DisplayString(string);
	// test loop
	while((GPIOA->IDR & (1)) == 0);
	while((GPIOA->IDR & (1)) != 0);
	//baseline loop
	baselineBPM = 0;
	baselineR = 0;
	n = 0;
	string = (uint8_t*)"base";
	LCD_DisplayString(string);
	while((GPIOA->IDR & (1)) == 0)
	{	
		//continually get the average of all the values to establish a baseline
		baselineBPM = (n*baselineBPM + getBPM())/(n+1); 
		baselineR = (n*baselineR + getResistivity())/(n+1);;
		n++;
	}
	
	while((GPIOA->IDR & (1)) != 0);
	
	while(1)			// main lie detector loop
	{
		if((GPIOA->IDR & (1)) != 0)
		{
			check = true;
			while((GPIOA->IDR & (1)) == 0);		// debounce
		}
		if((GPIOA->IDR & (1<<5))){
			displaySetting = (displaySetting + 1) % 3;
			while((GPIOA->IDR & (1<<5)) != 0);
		} 
		
		LCD_DisplayString(string);
		if(check)
		{
			inputR = getResistivity();
			inputBPM = getBPM();
			//mode 1, is it a lie or not?
			if(inputR > (baselineR+ threshR_comb) && inputBPM > (baselineBPM + threshBPM_comb))					//maybe adjust the 200....
			{
				string = (uint8_t*)"liesBR";
				//shoot nerf gun. Need to work out how long we want the motor to spin to shoot the gun
				truth = false;
				displaySetting = 0;
			}
			else if((inputBPM > baselineBPM + threshBPM))
			{
				string = (uint8_t*)"liesB";
				//shoot nerf gun. Need to work out how long we want the motor to spin to shoot the gun
				truth = false;
				displaySetting = 0;
			}
			else if((inputR > baselineR + threshR))
			{
				string = (uint8_t*)"liesR";
				truth = false;
				displaySetting = 0;
			}
			else
			{
				string = (uint8_t*)"true!  ";
				truth = true;
			}
		}
		if(displaySetting == 0)
		{
			if(truth)
				string = (uint8_t*)"true";
			else
				string = (uint8_t*)"lies";
		}
		else if(displaySetting == 1)		// BPM display state
		{
			//mode 2, bpm
			string = toString((int)(inputBPM - baselineBPM));
			//string = toString((int)(inputBPM));
		} 
		else if(displaySetting == 2)		// Resistivity difference display state
		{
			//string = toString((int)(inputBPM - baselineBPM));
			string = toString((int)(inputR - baselineR));
		}
		LCD_DisplayString(string);
		if(!truth && check)
		{
			//shoot();
			check = false;		// pause to be able to look at the conditions that triggered the lie state
		}
		
	}
}


void shoot()
{
	// pull the trigger
	for(int i = 0; i<3; i++)
 		tick_up();
	for(int i = 0; i<2; i++)
 		tick_down();
}

uint8_t * toString(int x)
{
	static uint8_t numString[4];
	int y;
	if(x < 0)
	{
		numString[0] = '-';
	} else {
		numString[0] = '0';
	}
	int z = abs(x);
	for(int i = 1; i < 4; i++)
	{
		y = z %10;
		z /= 10;
		numString[4 - i] = (y + 48);
	}
		
	return numString;
}
