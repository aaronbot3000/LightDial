/******* MS_Pot.c **************
 *
 * Use Fosc = 2 MHz for  Fcpu = 1 MHz.
 * Use Timer2 for a loop time of 10 ms.
 * Blink LED1 every two seconds.
 * Display startup message on LCD.
 * Display potentiometer output on LCD
 *******************************/

#include "p24HJ64GP502.h"
#include "stdint.h"
#include "string.h"

/*******************************
 * Configuration selections
 ******************************/
// FOSCSEL and FOSC default to FRC with divider and pin 10 = Fcpu
_FWDT(FWDTEN_OFF); 			// Turn off watchdog timer
_FICD(JTAGEN_OFF & ICS_PGD1);  // JTAG is disabled; ICD on PGC1, PGD1
_FOSC(IOL1WAY_OFF);

#define DATA_WORDS 18

uint16_t min_data[DATA_WORDS];
uint16_t hrs_data[DATA_WORDS];

void init();
void zero_dc();
void init_min();
void init_hrs();
void init_rtc();
void init_gssck();

void get_interface_state();
void update_display();
	void set_minute(uint16_t led, uint16_t power);
	void set_hour(uint16_t led, uint16_t power);

int DELAY;
int SCRATCH;

int delta_rpg = 0;
int b_min = 0;
int b_hrs = 0;

#define Delay(x) DELAY = x+1; while(--DELAY){ Nop(); Nop(); }
#define XBLNK LATBbits.LATB12
#define GSLAT_M LATBbits.LATB6
#define GSLAT_H LATBbits.LATB13

#define BUTTON_MIN PORTBbits.RB5
#define BUTTON_HRS PORTAbits.RA2
#define RPG0 PORTBbits.RB2
#define RPG1 PORTBbits.RB3

//////// Main program //////////////////////////////////////////

int main() {
	int minute = 0, hour = 0;
	int p_minute = 0, p_hour = 0;
	uint32_t counter = 0;
	init();
	XBLNK = 1;
	set_minute(0, 0x5FF);
	set_hour(0, 0xFFF);
	update_display();
	while(1) {
		get_interface_state();
		if (b_min) {
			if (delta_rpg > 0) {
				minute = (minute + 1) % 24;
			}
			else if (delta_rpg < 0) {
				minute = minute > 0 ? (minute - 1) : 23;
			}
		}
		else if (b_hrs) {
			if (delta_rpg > 0) {
				hour = (hour + 1) % 12;
			}
			else if (delta_rpg < 0) {
				hour = hour > 0 ? (hour - 1) : 11;
			}
		}
		if (counter >= 150000) {
			minute--;
			if (minute == 0) {
				hour--;
				if (hour < 0) {
					hour = 11;
				}
			}
			if (minute < 0) {
				minute = 23;
			}
			counter = 0;
		}
		if (minute != p_minute) {
			set_minute(p_minute, 0x000);
			set_minute(minute, 0x5FF);
			p_minute = minute;
			update_display();
		}
		if (hour != p_hour) {
			set_hour(p_hour, 0x000);
			set_hour(hour, 0xFFF);
			p_hour = hour;
			update_display();
		}
		counter++;
		Delay(100);
	}
}

/*******************************
 * Initialize oscillator and main clock
 * Set IO pins and output to zero
 * Initialize drivers' data to zero
 * Get initial RPG state
 * Initialize LED driver clock
 ******************************/
void init() {
	//__builtin_write_OSCCONH(0x07);
	
	//_IOLOCK =0;
	OSCCON = 0x0701;		// Use fast RCoscillator with Divide by 16
	CLKDIV = 0x0200;		// Divide by 4
	OSCTUN = 17;            // Tune for Fcpu = 1 MHz
	
	// unlock peripheral pin mapping
	__builtin_write_OSCCONL(0x46);
	__builtin_write_OSCCONL(0x57);
	__builtin_write_OSCCONL(0x17);

	TRISA = 0x0004;
	// Set up IO. 1 is input
	TRISB = 0x003F;
	AD1PCFGL = 0xFFFF;

	Nop();

	// Turn off drivers
	//LATB = 0x3040;
	XBLNK = 0;
	GSLAT_H = 1;
	GSLAT_M = 1;

	Delay(200);

	init_rtc();
	zero_dc();
	Delay(8000)
	init_min();
	init_hrs();
	init_gssck();
	// lock peripheral pin mapping
	__builtin_write_OSCCONL(0x46);
	__builtin_write_OSCCONL(0x57);
	__builtin_write_OSCCONL(0x57);

}

void init_gssck() {
	// Set up the driver clock
	// OC1 output 0x12
	RPOR4 |= 0x1200;
	// 50% duty cycle
	OC1R = 1;
	OC1RS = 2;
	// PWM mode with no fault protection
	OC1CON = 0x0006;
	// Set up Timer2
	PR2 = 2;
	TMR2 = 0;

	_T2IF = 0;
	_T2IE = 0;
	T2CON = 0x8000;
}

void zero_dc() {
	int i;
	// Connect a SPI peripheral to the DC pins
	// Clear and disable interrupts
	_SPI1IF = 0;
	_SPI1IE = 0;
	
	// SCK1 is 01000
	// SDO1 is 00111
	// Connect it to the output pins RP11 and RP10
	RPOR5 = 0x0807;

	// Turn it on in master mode
	SPI1CON1 = 0x013F;
	SPI1STATbits.SPIEN = 1;

	// Just in case
	Delay(10);

	// 17 bits user defined crap
	for (i = 0; i < 2; i++) {
		SPI1BUF = 0xAF;
		while (!SPI1STATbits.SPIRBF);
		SCRATCH = SPI1BUF;
	}
	// Function control
	// 1 0 0 1 1 1 1 1
	SPI1BUF = 0x9F;
	while (!SPI1STATbits.SPIRBF);
	SCRATCH = SPI1BUF;

	// 168 bits for brightness and dc
	for (i = 0; i < 24; i++) {
		SPI1BUF = 0xFF;
		while (!SPI1STATbits.SPIRBF);
		SCRATCH = SPI1BUF;
	}
	// Disconnect it from output pins
	SPI1STATbits.SPIEN = 0;
	RPOR5 = 0x0000;
}

/*******************************
 * Initialize current minute light
 * Set minute data
 ******************************/
void init_min() {
	int i;
	// Connect a SPI peripheral to the DC pins
	// Clear and disable interrupts
	_SPI1IF = 0;
	_SPI1IE = 0;
	
	// SDO1 is 00111
	// SCK1 is 01000
	// Connect it to the output pins RP8 and RP7
	RPOR4 |= 0x0007;
	RPOR3 |= 0x0800;

	// Turn it on in master mode, 2 byte data
	SPI1CON1 = 0x053F;
	SPI1STATbits.SPIEN = 1;

	Delay(10);
	
	GSLAT_M = 0;

	// 288 bits, or 18 byte pairs to set minutes
	for (i = 0; i < DATA_WORDS; i++) {
		SPI1BUF = 0x0000;
		while (!SPI1STATbits.SPIRBF);
		SCRATCH = SPI1BUF;
	}

	GSLAT_M = 1;
	memset(min_data, 0, sizeof(uint16_t) * DATA_WORDS);
}

/*******************************
 * Initialize current hour light
 * Set hour data
 ******************************/
void init_hrs() {
	int i;
	// Connect a SPI peripheral to the DC pins
	// Clear and disable interrupts
	_SPI2IF = 0;
	_SPI2IE = 0;
	
	// SDO2 is 01010
	// SCK2 is 01011
	// Connect it to the output pins RP15 and RP14
	RPOR7 |= 0x0A0B;

	// Turn it on in master mode, 2 byte data
	SPI2CON1 = 0x053F;
	SPI2STATbits.SPIEN = 1;
	
	Delay(10);

	GSLAT_H = 0;

	// 288 bits, or 18 byte pairs to set hours
	for (i = 0; i < DATA_WORDS; i++) {
		SPI2BUF = 0x0000;
		while (!SPI2STATbits.SPIRBF);
		SCRATCH = SPI2BUF;
	}

	GSLAT_H = 1;
	memset(hrs_data, 0, sizeof(uint16_t) * DATA_WORDS);
}

/*******************************
 * Initialize RTC and secondary oscillator
 * Set to some arbitrary time (1 Jan 2001 00:00:00)
 ******************************/
void init_rtc() {
	asm("MOV #0x55,W0");
	asm("MOV W0, NVMKEY");

	asm("MOV #0xAA,W0");
	asm("MOV W0, NVMKEY");
	_RTCWREN = 1;

	_RTCPTR = 0x3;
	RTCVAL = 0x0001;
	RTCVAL = 0x0101;
	RTCVAL = 0x0000;
	RTCVAL = 0x0000;

	_RTCEN = 1;
}

/*******************************
 * Get the state of the buttons and RPG
 ******************************/
void get_interface_state() {
	static uint16_t currRPG = 0;
	static uint16_t prevRPG = 0;
	static int dcount = 0;

	int r1, r2;

	//update currRPG and prevRPG
	prevRPG = currRPG;
	delta_rpg = 0;
	r1 = RPG0;
	r2 = RPG1;
	currRPG = ((r1 << 1) & 2) | (r2 & 1);

	//read button state into BUTTON_MIN	
	b_min = BUTTON_MIN;
	b_hrs = BUTTON_HRS;
	
	switch(prevRPG) {
		case 0:
			if(currRPG == 1) {
				dcount++;
			} else if(currRPG == 2) {
				dcount--;
			}
			break;
		case 1:
			if(currRPG == 3) {
				dcount++;
			} else if(currRPG == 0) {
				dcount--;
			}
			break;
		case 2:
			if(currRPG == 0) {
				dcount++;
			} else if(currRPG == 3) {
				dcount--;
			}
			break;
		case 3:
			if(currRPG == 2) {
				dcount++;
			} else if(currRPG == 1) {
				dcount--;
			}
			break;
	}
	if (dcount >= 4) {
		delta_rpg = 1;
		dcount = 0;
	}

	if (dcount <= -4) {
		delta_rpg = -1;
		dcount = 0;
	}
}

void set_minute(uint16_t led, uint16_t power) {
	uint16_t bit = (23 - led) * 12;
	uint16_t word = bit / 16;
	uint16_t startb = bit % 16;

	min_data[word] |= 0x0FFF << startb;
	min_data[word] &= (power << startb) | (~(0x0FFF << startb));

	if (startb > 4) {
		min_data[word + 1] |= 0x0FFF >> (16-startb);
		min_data[word + 1] &= (power >> (16 - startb)) | (~(0x0FFF >> (16 - startb)));
	}
}

void set_hour(uint16_t led, uint16_t power) {
	uint16_t bit = (23 - (led + 6)) * 12;
	uint16_t word = bit / 16;
	uint16_t startb = bit % 16;

	if (led >= 12)
		return;

	hrs_data[word] |= 0x0FFF << startb;
	hrs_data[word] &= (power << startb) | (~(0x0FFF << startb));

	if (startb > 4) {
		hrs_data[word + 1] |= 0x0FFF >> (16-startb);
		hrs_data[word + 1] &= (power >> (16 - startb)) | (~(0x0FFF >> (16 - startb)));
	}
}

void update_display() {
	int i;
	// send minutes
	GSLAT_M = 0;
	for (i = DATA_WORDS - 1; i >= 0; i--) {
		SPI1BUF = min_data[i];
		while (!SPI1STATbits.SPIRBF);
		SCRATCH = SPI1BUF;
	}
	GSLAT_M = 1;
	
	// send hours
	GSLAT_H = 0;
	for (i = DATA_WORDS - 1; i >= 0; i--) {
		SPI2BUF = hrs_data[i];
		while (!SPI2STATbits.SPIRBF);
		SCRATCH = SPI2BUF;
	}
	GSLAT_H = 1;
}
