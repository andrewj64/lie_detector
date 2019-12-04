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

int main(void){
	int displaySetting = 0;
	int n;
	float baselineBPM, baselineR;
	
	// Switch system clock to HSI here
 	RCC->CR |= RCC_CR_HSION;
	
	while((RCC->CR & RCC_CR_HSIRDY) == 0); //wait for ready bit
	
//	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
//	GPIOA->MODER &= ~0xF;
	
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
	motor_init(); //uses PB2,PB3,PB6,PB7
	set_speed(8000);	//for motor
	
	//initialize pa5 for the button overlay
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	
	GPIOA->MODER &= ~(0xC03);
	
	GPIOA->PUPDR &= ~(0xC03);
	GPIOA->PUPDR |= 0x802;
	
	uint8_t* string = (uint8_t*)"start";
	while(1)
	{
		//baseline loop
		baselineBPM = 0;
		baselineR = 0;
		n = 0;
		LCD_DisplayString(string);
		while((GPIOA->IDR & (1)) == 0)
		{	
			//continually get the average of all the values to establish a baseline
			baselineBPM = (n*baselineBPM + getBeats())/(n+1); 
			baselineR = (n*baselineR + getResult())/(n+1);;
			n++;
		}
		
		while((GPIOA->IDR & (1)) != 0);
		
		while((GPIOA->IDR & (1)) == 0)
		{
			
			if((GPIOA->IDR & (1<<5))){
				displaySetting = (displaySetting + 1) % 3;
				while((GPIOA->IDR & (1<<5)) != 0);
			}
			uint32_t input = getResult();
			uint32_t inputBPM = getBeats();
			LCD_DisplayString(string);
			
			if(displaySetting == 0){
				//mode 1, is it a lie or not?
				if(input > (baselineR+200))					//maybe adjust the 200....
				{
					string = (uint8_t*)"lies!  ";
				}
				else
				{
					string = (uint8_t*)"true!  ";
					//shoot nerf gun. Need to work out how long we want the motor to spin to shoot the gun
				}
			}else if(displaySetting == 1){
				//mode 2, bpm
				string = toString((int)(inputBPM - baselineBPM));

			} else {
				//do we want a mode 3 for the resistivity?
				string = toString((int)(input - baselineR));
			}
			
		}
		while((GPIOA->IDR & (1)) != 0);
	}
  

	
//	
//	while(1)
//	{
//		tick_up();
//		//for(int i = 0; i < 200000;i++);
//		//tick_down();
//	}
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
