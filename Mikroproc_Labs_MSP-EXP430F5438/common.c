/*
 * common.c
 *
 *	Some common stuff that doesn't not fit to particular example.
 *
 *  Created on: 11 Dec 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */

#include <msp430.h>

#include "driverlib.h"
#include "hal_MSP-EXP430F5438.h"

#include "common.h"

#define PWM_TIMER_PERIOD 400

uint8_t time_data[3];	// Stored data.
uint8_t buttons_pressed = 0;

/*
 * Parse the time from HH:MM:SS string.
 */
int parse_time(char* str) {
	int parsed_time[3];
	if (VALIDATE_DIGIT(str[0]) && VALIDATE_DIGIT(str[1])
			&& VALIDATE_DIGIT(str[3]) && VALIDATE_DIGIT(str[4])
			&& VALIDATE_DIGIT(str[6]) && VALIDATE_DIGIT(str[7])) {
		parsed_time[2] = (int) (str[0] - '0')*10 + (str[1] - '0');
		parsed_time[1] = (int) (str[3] - '0')*10 + (str[4] - '0');
		parsed_time[0] = (int) (str[6] - '0')*10 + (str[7] - '0');
		if (!INVALID_TIME(parsed_time)) {
			time_data[0] = parsed_time[0];
			time_data[1] = parsed_time[1];
			time_data[2] = parsed_time[2];
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

/*
 * Init LCD.
 */
void lcd_init() {
    halLcdInit();
    __delay_cycles(0x47FF);

	// Setup P8.3 for PWM.
	P8DIR = 0x08;
	P8OUT = 0x08;
	P8SEL = 0x08;

	// Setup Timer A0 for PWM.
	TA0CCR0 = PWM_TIMER_PERIOD;		// Set timer period.
	TA0CCR3 = PWM_TIMER_PERIOD / 2;	// Set ON period.
	TA0CCTL3 = OUTMOD_6;		// Output mode: PWM toggle/set.
	TA0CTL = TASSEL_2 + MC_1;	// SMCLK, count up to CCR0.

    halLcdSetContrast(100);
    halLcdClearScreen();
    halLcdPrint("Hi", 0);
}
