/*
 * sound_meter.c
 *
 *	Lab #6
 *	Demonstrates usage of ADC to read samples from the microphone
 *	found on the MSP-EXP430F5438 board.
 *	Calculating frequency and sound level based on the samples.
 *
 *  Created on: 9 Dec 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */

#include <stdio.h>
#include <math.h>
#include <msp430.h>

#include "driverlib.h"
#include "hal_lcd.h"
#include "hal_buttons.h"

#include "select.h"
#include "common.h"

#define PWM_TIMER_PERIOD 400
#define ENABLED_BUTTONS (BUTTON_S1 | BUTTON_S2)

char strbuf[255];
uint8_t redraw = 0;

#define ADC_MAX_LEVEL 0xFFF

// Values read from ADC source.
uint16_t last_adc_value = 0;
uint16_t max_adc_value = 0;
uint16_t min_adc_value = ADC_MAX_LEVEL;
uint16_t adc_probes = 0;

// Processed sample data.
uint16_t sound_prev_value = 0;
uint16_t sound_level_value = 0;
uint16_t sound_level_threshold = 2200;
uint16_t samples_above_threshold = 0;
uint16_t samples_below_threshold = 0;
uint16_t samples_peaks = 0;

/*
 * Init ADC.
 */
void adc_init() {
    UCSCTL8 |= MODOSCREQEN;
    ADC12CTL0 &= ~ADC12ENC;                // Ensure ENC is clear
    ADC12CTL0 = ADC12ON + ADC12SHT03 + ADC12SHT13 + ADC12MSC;
    ADC12CTL1 = ADC12SHP + ADC12CONSEQ_3 + ADC12SSEL_0;
    ADC12CTL2 = ADC12RES_2; // 12-bit output mode.
    ADC12MCTL1 = ADC12INCH_5 | ADC12EOS; // Select 5th channel.
    ADC12IE = 0x00;

    // Start conversion.
    ADC12CTL0 |= ADC12ENC | ADC12SC;
}

int sound_meter_main(void)
{
	float db_low, db_high;

	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	P1DIR = 0b00000011;
	P1OUT = 0x01;

	// Enable microphone (P6.4 - power, P6.5 - input).
	P6DIR = BIT4;
	P6OUT |= BIT4;
	P6OUT &= ~BIT5;
	P6SEL |= BIT5;

	// Event loop timer: 1 s.
	TB0CCTL0 = CCIE;
	TB0CCR0 = 32758;
	TB0CTL = TBSSEL_1				// Input source: ACLK.
			+ MC_1
			+ TBCLR;

	// Init stuff.
	lcd_init();
	adc_init();

	// Enable P2 interrupts for buttons.
	P2OUT = ENABLED_BUTTONS;
	P2DIR &= ~ENABLED_BUTTONS;
	P2REN = ENABLED_BUTTONS;
	P2SEL &= ~ENABLED_BUTTONS;
	P2IE = ENABLED_BUTTONS;
	P2IES = ENABLED_BUTTONS;
	P2IFG &= ~(ENABLED_BUTTONS);

	__bis_SR_register(GIE);

	while (1) {
		if (redraw) {
			// Calculate sound level value.
			db_low = 20 * log10f(min_adc_value / (float) ADC_MAX_LEVEL);
			db_high = 20 * log10f(max_adc_value / (float) ADC_MAX_LEVEL);

			// Print info.
			sprintf(strbuf, "%d Hz     ", samples_peaks);
			halLcdPrintLine(strbuf, 0, OVERWRITE_TEXT);
			sprintf(strbuf, "%.1f %.1f dB   ", db_low, db_high); // --printf-support=full required!
			halLcdPrintLine(strbuf, 1, OVERWRITE_TEXT);

		    sprintf(strbuf, "ADC min: %d      ", min_adc_value);
		    halLcdPrintLine(strbuf, 3, OVERWRITE_TEXT);
		    sprintf(strbuf, "ADC max: %d      ", max_adc_value);
		    halLcdPrintLine(strbuf, 4, OVERWRITE_TEXT);
		    sprintf(strbuf, "Samples: %d     ", adc_probes);
			halLcdPrintLine(strbuf, 5, OVERWRITE_TEXT);
			sprintf(strbuf, "Above: %d     ", samples_above_threshold);
			halLcdPrintLine(strbuf, 6, OVERWRITE_TEXT);
			sprintf(strbuf, "Below: %d     ", samples_below_threshold);
			halLcdPrintLine(strbuf, 7, OVERWRITE_TEXT);
			sprintf(strbuf, "Threshold: %d", sound_level_threshold);
			halLcdPrintLine(strbuf, 8, OVERWRITE_TEXT);

			redraw = 0;
		}
	}
}

#ifdef RUN_SOUND_METER

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
	if (P2IFG & ENABLED_BUTTONS) {
		buttons_pressed = P2IFG;
		// Buttons change threshold value.
		if (buttons_pressed & BUTTON_S1 && sound_level_threshold >= 100) {
			sound_level_threshold -= 100;
		}
		if (buttons_pressed & BUTTON_S2 && sound_level_threshold <= 3900) {
			sound_level_threshold += 100;
		}
	}
    P2IFG = 0;
}

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void) {
	P1OUT ^= 0x01;
	if (P1OUT & 0x01) {
		// Redraw mode.
		P1OUT &= ~0x02;
		ADC12IE = 0;
		redraw = 1;
	} else {
	    // Reset accumulated values.
	    min_adc_value = ADC_MAX_LEVEL;
	    max_adc_value = 0;
	    adc_probes = 0;
	    samples_above_threshold = 0;
	    samples_below_threshold = 0;
	    samples_peaks = 0;
		// Switch on measurement for one second.
		ADC12IE = BIT1;
	}
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
  	P1OUT ^= 0x02;
  	last_adc_value = ADC_MAX_LEVEL - (uint16_t) ADC12MEM1; // Invert value for further use.
  	if (last_adc_value > max_adc_value) {
  		max_adc_value = last_adc_value;
  	}
  	if (last_adc_value < min_adc_value) {
  		min_adc_value = last_adc_value;
  	}
  	++adc_probes;

  	sound_prev_value = sound_level_value;
  	sound_level_value = last_adc_value;
  	if (sound_level_value > sound_level_threshold) {
  		++samples_above_threshold;
  		if (sound_prev_value < sound_level_threshold) {
  			++samples_peaks;
  		}
  	} else {
  		++samples_below_threshold;
  	}
}

#endif
