/*
 * uart.c
 *
 *	Lab #5
 *	Demonstrates UART operation by allowing to read or set time via USB.
 *
 *  Created on: 2 Dec 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com
 */

#include <MSP430F5438.h>

#include "driverlib.h"

#include "select.h"
#include "common.h"
#include "flash.h"

#define USB_PORT_OUT      P5OUT
#define USB_PORT_SEL      P5SEL
#define USB_PORT_DIR      P5DIR
#define USB_PORT_REN      P5REN
#define USB_PIN_TXD       BIT6
#define USB_PIN_RXD       BIT7

volatile unsigned char buttonsPressed;

// Receive buffer.
char rcvbuff[255];
unsigned char receiveBufferSize = 0;

// Command buffer.
char cmdbuff[255];
unsigned char cmdbuff_size = 0;

/*
 * Fill the receive buffer with zeros.
 */
void clear_uart_buffer() {
	unsigned char i;

	for (i = 0; i < 255; i++)
		rcvbuff[i] = '\0';
}

/*
 * Initialize UART.
 */
void uart_init(void) {
	clear_uart_buffer();

	receiveBufferSize = 0;
	// Configure USB port.
	USB_PORT_SEL |= USB_PIN_RXD + USB_PIN_TXD;
	USB_PORT_DIR |= USB_PIN_TXD;
	USB_PORT_DIR &= ~USB_PIN_RXD;

	UCA1CTL1 |= UCSWRST;                    //Reset State

	UCA1CTL0 = UCMODE_0;
	UCA1CTL0 &= ~UC7BIT;					// Use 8-bit char.

	// Configure for Baud rate 9600.
    UCA1CTL1 |= UCSSEL__SMCLK;
    UCA1BR0 = 6;
    UCA1BR1 = 0x0;
    UCA1MCTL = UCOS16 | UCBRF_13;

    // Reset and enable interrupt.
	UCA1CTL1 &= ~UCSWRST;
	UCA1IE |= UCRXIE;
}

/*
 * Transmit single character.
 */
void uart_send_char(char character)
{
    while (!(UCA1IFG & UCTXIFG)) ;
    UCA1TXBUF = character;
}

/*
 * Transmit string.
 */
void uart_send_str(char* string, int length) {
	int i;
	for (i = 0; i < length; i++) {
		uart_send_char(string[i]);
	}
}

/*
 * Convert time data to string (faster than sprintf).
 */
void print_time(int8_t* data) {
	char time_str[] = "12:34:56\r\n";

	time_str[7] = '0' + data[0] % 10;
	time_str[6] = '0' + data[0] / 10;
	time_str[4] = '0' + data[1] % 10;
	time_str[3] = '0' + data[1] / 10;
	time_str[1] = '0' + data[2] % 10;
	time_str[0] = '0' + data[2] / 10;

	uart_send_str(time_str, 10);
}

int uart_main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

    unsigned char ch;
    volatile unsigned char i;

	P1DIR = 0b00000011;				// Set port 1 pins 0 & 1 to output.
	P1OUT = 0x00;					// Set ON value to port 1 pin 0.
	P8DIR = 0b00001000;
	P8OUT = 0x00;

	// Setup timer A0 to count exactly each second.
	TA0CCTL0 = CCIE;
	TA0CCR0 = 32758;
	TA0CTL = TASSEL_1				// Input source: ACLK.
			+ MC_1
			+ TACLR;

	load_and_reset();
	uart_init();

	// Tell them we're ready.
	uart_send_str("OK\r\n", 4);

    __bis_SR_register(GIE);

    while (1) {
    	__delay_cycles(0x47FF);

    	while (receiveBufferSize > 0) {
    		// Move char from receive to command buffer and echo it.
    		ch = rcvbuff[--receiveBufferSize];
    		cmdbuff[cmdbuff_size++] = ch;
    		uart_send_char(ch);

    		// Process command entry.
    		if (ch == 0x0D) {
    			uart_send_char('\r');
    			uart_send_char('\n');

    			// Try to get time from command.
    			if (!parse_time(cmdbuff)) {
    				print_time(time_data);
    			}

    			cmdbuff_size = 0;
    		}
    	}

    	save_time(time_data);
    }
}

/*
 * Required in order to make sure that ISRs are linked only once.
 * Remove this ifdef statement if example will be used separately.
 */
#ifdef RUN_UART

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
	if (P1OUT & 0x01) {
		P1OUT ^= 0x01;
	}
	if (P1OUT & 0x02) {
		P1OUT ^= 0x02;
	}
	if (P8OUT & 0x08) {
		P8OUT ^= 0x08;
	}

	if(++time_data[0] >= 60) {
		time_data[0] = 0;
		if (++time_data[1] >= 60) {
			time_data[1] = 0;
			P8OUT ^= 0x08;
			if (++time_data[2] >= 24) {
				time_data[2] = 0;
			}
		}

		P1OUT ^= 0x02;
	}

	if (time_data[0] % 5 == 0) {
		P1OUT ^= 0x01;
	}
}

#endif

#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    rcvbuff[receiveBufferSize++] = UCA1RXBUF;
    __bic_SR_register_on_exit(LPM3_bits);
}
