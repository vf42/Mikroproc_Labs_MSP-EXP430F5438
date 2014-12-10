/*
 * timers.c
 *
 * Lab #3
 * Demonstrates PWM usage for LCD backlight brightness control.
 *
 *  Created on: 01 Dec 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */

#include "msp430.h"

#include "pwm.h"

#define TIMER_PERIOD 400

int direction = 1; // Specifies brightness change direction: up/down.

int pwm_main(void) {
    WDTCTL = WDTPW + WDTHOLD;

	P1DIR = 0x03;
	P1OUT = 0x01;

	// Setup P8.3 for PWM.
	P8DIR = 0x08;
	P8OUT = 0x08;
	P8SEL = 0x08;

	// Setup Timer A0 for PWM.
	TA0CCR0 = TIMER_PERIOD;		// Set timer period.
	TA0CCR3 = TIMER_PERIOD / 2;	// Set ON period.
	TA0CCTL3 = OUTMOD_6;		// Output mode: PWM toggle/set.
	TA0CTL = TASSEL_2 + MC_1;	// SMCLK, count up to CCR0.

	// Setup timer B0 to count 1/8 seconds.
	TB0CCTL0 = CCIE;
	TB0CCR0 = 32758 / 8;
	TB0CTL = TBSSEL_1
			+ MC_1
			+ TBCLR;

    __bis_SR_register(LPM0_bits + GIE);

    return 0;
}

/*
 * Required in order to make sure that ISRs are linked only once.
 * Remove this ifdef statement if example will be used separately.
 */
#ifdef RUN_PWM

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void) {
	P1OUT ^= 0x03;
	TA0CCR3 += 10 * direction;
	if (TA0CCR3 <= 10 || TA0CCR3 >= TA0CCR0 - 10) {
		direction *= -1;
	}
}

#endif
