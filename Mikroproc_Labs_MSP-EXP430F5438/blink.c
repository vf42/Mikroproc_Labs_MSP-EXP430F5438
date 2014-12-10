/*
 * blink.c
 *
 * Lab #1
 * Demonstrates simple blinking using red and yellow LEDs (P1.0 & P1.1)
 * and the LCD screen's backlight (P8.3).
 *
 *  Created on: 18 Sep 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */

#include "msp430.h"

#include "select.h"

int blink_main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	P1DIR = 0b00000011;				// Set port 1 pins 0 & 1 to output.
	P1OUT = 0x01;					// Set ON value to port 1 pin 0.

	P8DIR = 0b00001000;				// Set port 8 pin 3 to output.
	P8OUT = 0b00001000;				// Set ON value to port 8 pin 3.

	volatile unsigned int k;
	for(k = 0; ; k++) {
		volatile unsigned int i;
		volatile unsigned int step = 1000 + (k / 1000);

		P8OUT ^= 0x08;				// Toggle port 8 pin 3 value.
		for (i = 0; i < (step * 2); i++) {
			if (i % step == 0)
				P1OUT ^= 0x03;		// Toggle port 1 pin 0 & 1 values.
		}

		if (k > 50000) {			// Sleep for some undefined time.
			k = 0;
		}
	}
}
