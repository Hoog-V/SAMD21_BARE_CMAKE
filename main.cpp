#include "sam.h"

// Constants for Clock Generators
#define GENERIC_CLOCK_GENERATOR_0   (0u)
#define GENERIC_CLOCK_GENERATOR_1   (1u)

//Constants for clock identifiers
#define CLOCK_XOSC32K	0x05
#define CLOCK_DFLL48	0x07
#define CLOCK_8MHZ		0x06

//function for setting related to power management system
//See section 16 of the datasheet
void PM_Clock_Bus_Setup(void) {
	//in power management system do not divide system clock down
	PM->CPUSEL.reg  = PM_CPUSEL_CPUDIV_DIV1; 
	PM->APBASEL.reg = PM_APBASEL_APBADIV_DIV1_Val;
	PM->APBBSEL.reg = PM_APBBSEL_APBBDIV_DIV1_Val;
	PM->APBCSEL.reg = PM_APBCSEL_APBCDIV_DIV1_Val;
} 

void Clock_Init(void) {
	
	//NVM CTRLB registers and RWS[3:0] bit for setting wait states fro a read operation	//Defaults to 0 and can go as high as 15 (4 bits)
	NVMCTRL->CTRLB.bit.RWS = 1;		// 1 wait state required @ 3.3V & 48MHz
	
	//the system controller subsystem controls the clocks. The XOSC32K register sets up the External 32.768kHz oscillator
	SYSCTRL->XOSC32K.bit.WRTLOCK = 0;		//XOSC32K configuration is not locked
	SYSCTRL->XOSC32K.bit.STARTUP = 0x2;		//3 cycle start-up time
	SYSCTRL->XOSC32K.bit.ONDEMAND = 0;		//Osc. is always running when enabled
	SYSCTRL->XOSC32K.bit.RUNSTDBY = 0;		//Osc. is disabled in standby sleep mode
	SYSCTRL->XOSC32K.bit.AAMPEN = 0;		//Disable automatic amplitude control
	SYSCTRL->XOSC32K.bit.EN32K = 1;			// 32kHz output is enable
	SYSCTRL->XOSC32K.bit.XTALEN = 1;		// Crystal connected to XIN32/XOUT32
	// Enable the Oscillator - Separate step per data sheet recommendation (sec 17.6.3)
	SYSCTRL->XOSC32K.bit.ENABLE = 1; //should this be moved after the sync????
	// Wait for XOSC32K to stabilize
	while(!SYSCTRL->PCLKSR.bit.XOSC32KRDY);
	
	//Generic clock subsystem setting GENDIV register to set the divide factor for Generic clock 1
	GCLK->GENDIV.reg |= GCLK_GENDIV_DIV(1) | GCLK_GENDIV_ID(GENERIC_CLOCK_GENERATOR_1); //set divide factor for gen clock 1
	
	// Configure Generic Clock Generator 1 with XOSC32K as source
	GCLK->GENCTRL.bit.RUNSTDBY = 0; // Generic Clock Generator is stopped in stdby
	GCLK->GENCTRL.bit.DIVSEL = 0;   // enable clock divide
	GCLK->GENCTRL.bit.OE = 0;		// Disable generator output to GCLK_IO[1]
	GCLK->GENCTRL.bit.OOV = 0;		// We will not use this signal as an output
	GCLK->GENCTRL.bit.IDC = 1;		// Generator duty cycle is 50/50
	GCLK->GENCTRL.bit.GENEN = 1;	// Enable the generator
	GCLK->GENCTRL.bit.SRC = CLOCK_XOSC32K;	// Generator source: XOSC32K output
	GCLK->GENCTRL.bit.ID = GENERIC_CLOCK_GENERATOR_1;	// This was created in Definitions.h, refers to generic clock 1
	// GENCTRL is Write-Synchronized...so wait for write to complete
	while(GCLK->STATUS.bit.SYNCBUSY);
	
	//			Enable the Generic Clock     // Generic Clock Generator 1 is the source			 Generic Clock Multiplexer 0 (DFLL48M Reference)
	GCLK->CLKCTRL.reg |= GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(GENERIC_CLOCK_GENERATOR_1) | GCLK_CLKCTRL_ID_DFLL48;
	
	// DFLL Configuration in Closed Loop mode, cf product data sheet chapter
	// 17.6.7.1 - Closed-Loop Operation
	// Enable the DFLL48M in open loop mode. Without this step, attempts to go into closed loop mode at 48 MHz will
	// result in Processor Reset (you'll be at the in the Reset_Handler in startup_samd21.c).
	// PCLKSR.DFLLRDY must be one before writing to the DFLL Control register
	// Note that the DFLLRDY bit represents status of register synchronization - NOT clock stability
	// (see Data Sheet 17.6.14 Synchronization for detail)
	while(!SYSCTRL->PCLKSR.bit.DFLLRDY);
	SYSCTRL->DFLLCTRL.reg = (uint16_t)(SYSCTRL_DFLLCTRL_ENABLE);
	while(!SYSCTRL->PCLKSR.bit.DFLLRDY);
	
	// Set up the Multiplier, Coarse and Fine steps. These values help the clock lock at the set frequency
	//There is not much information in the datasheet on what they do exactly and how to tune them for your specific needs
	//lower values lead to more "overshoot" but faster frequency lock. Higher values lead to less "overshoot" but slower lock time
	//Datasheet says put them at half to get best of both worlds
	SYSCTRL->DFLLMUL.bit.CSTEP = 31; //max value is 2^6 - 1 or 63
	SYSCTRL->DFLLMUL.bit.FSTEP = 511; //max value is 2^10 - 1 or 1023
	SYSCTRL->DFLLMUL.bit.MUL = 1465; //multiplier of ref external clock to get to 48M --> 32768 x 1465 = 48,005,120
	// Wait for synchronization
	while(!SYSCTRL->PCLKSR.bit.DFLLRDY);
	// To reduce lock time, load factory calibrated values into DFLLVAL (cf. Data Sheet 17.6.7.1)
	// Location of value is defined in Data Sheet Table 10-5. NVM Software Calibration Area Mapping
	
	// Switch DFLL48M to Closed Loop mode and enable WAITLOCK
	SYSCTRL->DFLLCTRL.reg |= (uint16_t) (SYSCTRL_DFLLCTRL_MODE | SYSCTRL_DFLLCTRL_WAITLOCK); 

	// Now that DFLL48M is running, switch CLKGEN0 source to it to run the core at 48 MHz.
	// Enable output of Generic Clock Generator 0 (GCLK_MAIN) to the GCLK_IO[0] GPIO Pin
	GCLK->GENCTRL.bit.RUNSTDBY = 0;		// Generic Clock Generator is stopped in stdby
	GCLK->GENCTRL.bit.DIVSEL = 0;		// Use GENDIV.DIV value to divide the generator
	GCLK->GENCTRL.bit.OE = 0;			// Enable generator output to GCLK_IO[0]
	GCLK->GENCTRL.bit.OOV = 0;			// GCLK_IO[0] output value when generator is off
	GCLK->GENCTRL.bit.IDC = 1;			// Generator duty cycle is 50/50
	GCLK->GENCTRL.bit.GENEN = 1;		// Enable the generator
	//The next two lines are where we set the system clock
	GCLK->GENCTRL.bit.SRC = CLOCK_DFLL48;		// Generator source: DFLL48M output
	GCLK->GENCTRL.bit.ID = GENERIC_CLOCK_GENERATOR_0;	// Generic clock gen 0 is used for system clock
	// GENCTRL is Write-Synchronized...so wait for write to complete
	while(GCLK->STATUS.bit.SYNCBUSY);
	
	//setup the built-in 8MHz clock
	SYSCTRL->OSC8M.bit.PRESC = 0;		// Prescale by 1 (no divide)
	SYSCTRL->OSC8M.bit.ONDEMAND = 0;	// Oscillator is always on if enabled

	PM_Clock_Bus_Setup(); //setup power management system
}


int main(void)
{
	Clock_Init();
	/* Initializes MCU, drivers and middleware */
	//atmel_start_init();
	/* Replace with your application code */
	PORT->Group[0].DIRSET.reg |= 1 << 2;
	PORT->Group[0].OUTCLR.reg |= 1 << 2;
	while (1) {
        PORT->Group[0].OUTTGL.reg |= 1 << 2;
		//PORT->Group[0].OUTTGL.reg |= 1 << 2;
		//delay_ms(500);
	}
}
