/*
 * timers.c
 *
 * Lab #2
 * Demonstrates usage of different timers for blinking.
 *
 *  Created on: 02 Oct 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */

#include "msp430.h"

#include "select.h"

int timers_main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	P1DIR = 0b00000011;				// Set port 1 pins 0 & 1 to output.
	P1OUT = 0x01;					// Set ON value to port 1 pin 0.
	P8DIR = 0b00001000;				// Set port 8 pin 3 to output.
	P8OUT = 0b00000000;				// Set ON value to port 8 pin 3.

	// Setup timer A1 to count approx each second.
	TA1CCTL0 = CCIE;				// CCR0 interrupt enabled.
	TA1CCR0 = 32768;				// Set target value.
	TA1EX0 = TAIDEX_3;				// Secondary divider: /4.
	TA1CTL = TASSEL_2 				// Input source: SMCLK.
			+ MC_1					// Timer mode: count up to target value.
			+ ID__8					// Primary divider: /8.
			+ TACLR;				// Clear TAR.

	// Setup timer A0 to count exactly each second.
	TA0CCTL0 = CCIE;
	TA0CCR0 = 32758;
	TA0CTL = TASSEL_1				// Input source: ACLK.
			+ MC_1
			+ TACLR;

	// Setup timer B0 to count exactly 5 seconds.
	TB0CCTL0 = CCIE;
	TB0CCR0 = 32758;
	TB0EX0 = TAIDEX_4;				// Secondary divider /5.
	TB0CTL = TBSSEL_1				// Input source: ACLK.
			+ MC_1
			+ TBCLR;

	__bis_SR_register(LPM0_bits + GIE);

	return 0;
}

/*
 * Required in order to make sure that ISRs are linked only once.
 * Remove this ifdef statement if example will be used separately.
 */
#ifdef RUN_TIMERS

#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void) {
	P1OUT ^= 0x01;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
	P1OUT ^= 0x02;
}

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void) {
	P8OUT ^= 0x08;
}

#endif
