#include "stm32l476xx.h"
#include "Systick.h"
#include "LCD.h"
#include "keypad.h"
#include "motor.h"
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
*		Set up HSI clock
*		Initialize keypad GPIO
*		loop forever
*			Loop until #
*				Scan for key press
*				make sure pressed key is valid
*				store keypress in counter variable
*			EndLoop
*			convert count to an int 
*			make sure the number is less than 59. If not, make 59
*			wind up clock (probs a loop that adds and ticks clock)
*			wait until # is pressed again (loop)
* 		loop until counter variable is zero
*				decrement motor by one tick
*				if counter variable % 15 == 0
*					tick another time
*				counter variable--
*			endloop
*			turn on buzzer for a sec
*		endloop
* END
*			
*
*/


int main(void){
	
	// Switch system clock to HSI here
	RCC->CR |= RCC_CR_HSION;
	
	while((RCC->CR & RCC_CR_HSIRDY) == 0); //wait for ready bit
	
	SysTick_Init();
	LCD_Initialization();
	keypad_pin_init();
	motor_init();
	
		// Enable GPIO Clock for buzzer
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
	
	// GPIO Mode: Input(00), Output (01),
	// AF(10), Analog (11)
	GPIOE->MODER &= ~0x30000U;		// clear PE8 MODER (buzzer)
	GPIOE->MODER |=  0x10000U;			// set PE8 to output
	
	// GPIO Push-Pull: No pull-up pull-down (00),
	// Pull-up (01), Pull-down (10), Reserved (11)
//	GPIOE->PUPDR &= ~0xC000U;
//	GPIOE->PUPDR |= 0x8000U; // Pull down
	
	//GPIOE->OTYPER &= ~100U;		// set PE8 to push-pull
	set_speed(8000);
	
	while(1)
	{
		uint8_t input = '0';
		uint8_t count [3] = {0};
		uint32_t index = 0;
		while(input != 0x23)
		{
			input = keypad_scan();									//read in input
			if(input >= '0' && input <= '9' && index < 2)				//check if valid
			{
				count[index] = input;											//set as a digit
				index++;
			}
		}
		LCD_DisplayString(count);
		uint32_t countTime = (count[1] - 48) + (count[0]- 48)*10;				//convert count to an interger
		if(countTime > 59)												//if countTime is bigger than 59, just go ahead and fix that
		{
			countTime = 59;													//make it 59
		}

		wind_up(countTime);					//loop to wind up clock
		
		input = 0;
		while(input != 0x23)				//wait loop to wait for # press
		{
			input = keypad_scan();									//read in input
		}

		setSec(countTime);
		buzzOn();
	}
}


