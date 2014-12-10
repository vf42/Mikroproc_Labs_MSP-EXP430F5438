/*
 * mem_ptr.c
 *
 * Lab #4
 * Demonstrates usage of Flash memory to store time data.
 * Two flash segments are used (SEG0, SEG1) in order to always have one
 * active copy of data, as flash segment needs to be erased before writing.
 *
 *  Created on: 30 Oct 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */


#include "msp430.h"

#include "driverlib.h"

#include "select.h"
#include "common.h"

uint8_t curr_seg = 0;	// Segment selection.

// Segment adresses.
#define SEG0_ADDR   (0x1800)
#define SEG1_ADDR   (0x1880)

/*
 * Save the data to next flash segment, clear previous one.
 */
void save_time(uint8_t* data) {
	uint8_t status;
	uint32_t seg_write;
	uint32_t seg_erase;

	// Swap write/erase segments.
	seg_write = curr_seg ? SEG1_ADDR : SEG0_ADDR;
	seg_erase = curr_seg ? SEG0_ADDR : SEG1_ADDR;
	curr_seg = !curr_seg;

	FLASH_write8(data, (uint8_t*) (seg_write), 3);
	do {
		FLASH_segmentErase((uint8_t*) seg_erase);
		status = FLASH_eraseCheck((uint8_t*) seg_erase, 128);
	} while (status == STATUS_FAIL);
}

/*
 * Read the data from specified segment.
 */
void read_time(uint8_t* segment, uint8_t* target) {
	target[0] = *(uint8_t*)(segment) + 5;
	target[1] = *(uint8_t*)(segment + sizeof(uint8_t));
	target[2] = *(uint8_t*)(segment + 2*sizeof(uint8_t));
}

/*
 * Try to load the data from flash, initialize it with zeros if failed.
 * Clear segments and re-save the data afterwards.
 */
void load_and_reset() {
	uint8_t status;
	// Load data from 1st segment.
	read_time((uint8_t*) SEG0_ADDR, (uint8_t*) time_data);
	// If something doesn't look ok - try to read from 2nd curr_segment.
	if (INVALID_TIME(time_data)) {
		read_time((uint8_t*) SEG1_ADDR, (uint8_t*) time_data);
	}
	// Still not ok - reset.
	if (INVALID_TIME(time_data)) {
		time_data[0] = 0;
		time_data[1] = 0;
		time_data[2] = 0;
	}
	// Clear both segments data and write to the first one.
	do {
		FLASH_segmentErase((uint8_t*) SEG0_ADDR);
		status = FLASH_eraseCheck((uint8_t*) SEG0_ADDR, 128);
	} while (status == STATUS_FAIL);
	do {
		FLASH_segmentErase((uint8_t*) SEG1_ADDR);
		status = FLASH_eraseCheck((uint8_t*) SEG1_ADDR, 128);
	} while (status == STATUS_FAIL);
	save_time(time_data);
}

int flash_main(void) {
	int i;

	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	P1DIR = 0b00000011;				// Set port 1 pins 0 & 1 to output.
	P1OUT = 0x00;					// Set ON value to port 1 pin 0.
	P8DIR = 0b00001000;
	P8OUT = 0x00;

	load_and_reset();

	// Setup timer A0 to count exactly each second.
	TA0CCTL0 = CCIE;
	TA0CCR0 = 32758;
	TA0CTL = TASSEL_1				// Input source: ACLK.
			+ MC_1
			+ TACLR;

	__bis_SR_register(GIE);

	while (1) {
		for (i = 0; i < 20000; i++) { }
		save_time(time_data);
	}
}

/*
 * Required in order to make sure that ISRs are linked only once.
 * Remove this ifdef statement if example will be used separately.
 */
#ifdef RUN_FLASH

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
	/*
	 * Blinks red LED once in 5 seconds.
	 * Blinks yellow LED once in minute.
	 * Blinks LCD backlight once in hour.
	 */

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
