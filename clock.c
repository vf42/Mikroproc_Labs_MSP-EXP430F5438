/*
 * clock.c
 *
 *	Demo application: clock.
 *
 *  Created on: 01 Dec 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */


#include <msp430.h>

#include "driverlib.h"
#include "hal_MSP-EXP430F5438.h"

#include "select.h"
#include "common.h"
#include "uart.h"
#include "flash.h"

#define USB_PORT_OUT      P5OUT
#define USB_PORT_SEL      P5SEL
#define USB_PORT_DIR      P5DIR
#define USB_PORT_REN      P5REN
#define USB_PIN_TXD       BIT6
#define USB_PIN_RXD       BIT7

#define BRIGHTNESS_STEP 40

#define BUTTON_BITS (BIT6 | BIT7 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5)

#define ADJUST_HOUR_UP (0x01)
#define ADJUST_HOUR_DOWN (0x02)
#define ADJUST_MINUTE_UP (0x04)
#define ADJUST_MINUTE_DOWN (0x08)
#define ADJUST_BRIGHTNESS_UP (0x10)
#define ADJUST_BRIGHTNESS_DOWN (0x20)
#define ADJUST_SECONDS_RESET (0x40)

#define INVALID_TIME(td) (td[0] >= 60 || td[1] >= 60 || td[2] >= 24)
#define VALIDATE_DIGIT(d) (d >= '0' && d <= '9')

uint8_t time_adjustment = 0;

char* status_str = "";

void render_time(uint8_t* data) {
	char time_str[] = "12:34:56";

	time_str[7] = '0' + data[0] % 10;
	time_str[6] = '0' + data[0] / 10;
	time_str[4] = '0' + data[1] % 10;
	time_str[3] = '0' + data[1] / 10;
	time_str[1] = '0' + data[2] % 10;
	time_str[0] = '0' + data[2] / 10;

    halLcdClearScreen();
	halLcdPrintXY(time_str, 40, 45, GRAYSCALE_TEXT | OVERWRITE_TEXT);
	halLcdPrintXY(status_str, 0, 98, GRAYSCALE_TEXT | OVERWRITE_TEXT | INVERT_TEXT);
}

int clock_main(void) {
	char ch;

	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	P1DIR = 0b00000011;
	P1OUT = 0x00;

	// Enable P2 interrupts for buttons.
	P2OUT = BUTTON_BITS;
	P2DIR &= ~BUTTON_BITS;
	P2REN = BUTTON_BITS;
	P2SEL &= ~BUTTON_BITS;
	P2IE = BUTTON_BITS;
	P2IES = BUTTON_BITS;
	P2IFG &= ~(BUTTON_BITS);

	load_and_reset();
	uart_init();
	lcd_init();

	// Setup timer B0 to count exactly each second.
	TB0CCTL0 = CCIE;
	TB0CCR0 = 32758;
	TB0CTL = TBSSEL_1				// Input source: ACLK.
			+ MC_1
			+ TBCLR;

	__bis_SR_register(GIE);

	while (1) {
		__delay_cycles(0x47FF);

		while (receiveBufferSize > 0) {
			ch = rcvbuff[--receiveBufferSize];
			cmdbuff[cmdbuff_size++] = ch;
			uart_send_char(ch);
			if (ch == 0x0D) {
				//uart_send_char('\r');
				uart_send_char('\n');
				parse_time(cmdbuff);
				cmdbuff_size = 0;
			}
		}
	}
}

#ifdef RUN_CLOCK

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void) {
	// Switch off button interrupts.
	P2IE &= ~BUTTON_BITS;

	// Reset leds.
	if (P1OUT & 0x01) {
		P1OUT ^= 0x01;
	}
	if (P1OUT & 0x02) {
		P1OUT ^= 0x02;
	}

	// Adjust time.
	if (time_adjustment & ADJUST_MINUTE_UP) {
		time_adjustment ^= ADJUST_MINUTE_UP;
		if (++time_data[1] >= 60) {
			time_data[1] = 0;
		}
	}
	if (time_adjustment & ADJUST_MINUTE_DOWN) {
		time_adjustment ^= ADJUST_MINUTE_DOWN;
		if (time_data[1] == 0) {
			time_data[1] = 59;
		} else {
			--time_data[1];
		}
	}
	if (time_adjustment & ADJUST_HOUR_UP) {
		time_adjustment ^= ADJUST_HOUR_UP;
		if (++time_data[2] >= 24) {
			time_data[2] = 0;
		}
	}
	if (time_adjustment & ADJUST_HOUR_DOWN) {
		time_adjustment ^= ADJUST_HOUR_DOWN;
		if (time_data[2] == 0) {
			time_data[2] = 23;
		} else {
			--time_data[2];
		}
	}
	if (time_adjustment & ADJUST_BRIGHTNESS_DOWN) {
		time_adjustment ^= ADJUST_BRIGHTNESS_DOWN;
		if (TA0CCR3 >= BRIGHTNESS_STEP) {
			TA0CCR3 -= BRIGHTNESS_STEP;
		} else {
			TA0CCR3 = 0;
		}
	}
	if (time_adjustment & ADJUST_BRIGHTNESS_UP) {
		time_adjustment ^= ADJUST_BRIGHTNESS_UP;
		TA0CCR3 += BRIGHTNESS_STEP;
		if (TA0CCR3 >= TA0CCR0) {
			TA0CCR3 = TA0CCR0;
		}
	}
	if (time_adjustment & ADJUST_SECONDS_RESET) {
		time_adjustment ^= ADJUST_SECONDS_RESET;
		time_data[0] = 0;
	}

	// Count time.
	if(++time_data[0] >= 60) {
		time_data[0] = 0;
		if (++time_data[1] >= 60) {
			time_data[1] = 0;
			if (++time_data[2] >= 24) {
				time_data[2] = 0;
			}
		}
		P1OUT ^= 0x02;
	}

	if (time_data[0] % 5 == 0) {
		P1OUT ^= 0x01;
	}

	render_time(time_data);
	save_time(time_data);

	// Reset status.
	time_adjustment = 0;
	status_str = "";
	// Switch button interrupts on.
	P2IE |= BUTTON_BITS;
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
	// Poll buttons.
	if (!(P2IN & BIT6) && !time_adjustment) {
		time_adjustment |= ADJUST_BRIGHTNESS_DOWN;
		status_str = "Brightness Down";
	}
	else if (!(P2IN & BIT7) && !time_adjustment) {
		time_adjustment |= ADJUST_BRIGHTNESS_UP;
		status_str = "Brightness Up";
	}
	else if (!(P2IN & BIT1) && !time_adjustment) {
		time_adjustment |= ADJUST_HOUR_DOWN;
		status_str = "Hour Down";
	}
	else if (!(P2IN & BIT2) && !time_adjustment) {
		time_adjustment |= ADJUST_HOUR_UP;
		status_str = "Hour Up";
	}
	else if (!(P2IN & BIT5) && !time_adjustment) {
		time_adjustment |= ADJUST_MINUTE_DOWN;
		status_str = "Minute Down";
	}
	else if (!(P2IN & BIT4) && !time_adjustment) {
		time_adjustment |= ADJUST_MINUTE_UP;
		status_str = "Minute Up";
	}
	else if (!(P2IN & BIT3) && !time_adjustment) {
		time_adjustment |= ADJUST_SECONDS_RESET;
		status_str = "Reset Seconds";
	}
}

#endif
