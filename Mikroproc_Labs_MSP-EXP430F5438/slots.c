/*
 * slots.c
 *
 * Simple slot machine demo app.
 *
 *  Created on: 3 Dec 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */

#include <stdio.h>
#include <msp430.h>

#include "driverlib.h"
#include "hal_lcd.h"
#include "hal_buttons.h"

#include "select.h"
#include "common.h"

/*
 * Stub-pseudo-RNG based on Fibonacci sequence.
 */
uint8_t seed1 = 1;
uint8_t seed2 = 1;

int rand() {
	int old = seed1;
	seed1 = seed2;
	seed2 += old;
	if (seed1 > seed2) {
		seed1 = 1;
		seed2 = 1;
	}
	return old;
}


/*
 * Slot machine states.
 */
char reel1[10] = "JQAKQJAKJQ";
uint8_t reel1_len = 8;
char reel2[11] = "QAJQKJQKJQA";
uint8_t reel2_len = 9;
char reel3[11] = "KJAQKAQJAKJ";
uint8_t reel3_len = 9;

uint8_t reel1_p = 0;
uint8_t reel2_p = 0;
uint8_t reel3_p = 0;

char window[9];
uint8_t won_symbols[9];
uint8_t lines[5];
uint8_t line_payouts[5];
uint8_t won_lines = 0;
uint8_t total_payout = 0;

void gen_slot_window() {
	reel1_p = (reel1_p + rand()) % reel1_len;
	reel2_p = (reel2_p + rand()) % reel2_len;
	reel3_p = (reel3_p + rand()) % reel3_len;
	int8_t i;
	for (i = 2; i >= 0; i--) {
		window[i*3 + 0] = reel1[reel1_p + i];
		window[i*3 + 1] = reel2[reel2_p + i];
		window[i*3 + 2] = reel3[reel3_p + i];
	}
}

int payout_for_symbol(char symbol) {
	switch (symbol) {
	case 'J': return 10;
	case 'Q': return 25;
	case 'K': return 35;
	case 'A': return 50;
	}
}

void calc_line_winnings() {
	int8_t i;
	for (i = 8 ; i >= 0; i--) {
		won_symbols[i] = 0;
	}

	lines[0] = (window[0] ==  window[1]) && (window[1] == window[2]);
	line_payouts[0] = lines[0] * payout_for_symbol(window[0]);
	won_symbols[0] |= lines[0];
	won_symbols[1] |= lines[0];
	won_symbols[2] |= lines[0];

	lines[1] = (window[3] ==  window[4]) && (window[4] == window[5]);
	line_payouts[1] = lines[1] * payout_for_symbol(window[3]);
	won_symbols[3] |= lines[1];
	won_symbols[4] |= lines[1];
	won_symbols[5] |= lines[1];

	lines[2] = (window[6] ==  window[7]) && (window[7] == window[8]);
	line_payouts[2] = lines[2] * payout_for_symbol(window[6]);
	won_symbols[6] |= lines[2];
	won_symbols[7] |= lines[2];
	won_symbols[8] |= lines[2];

	lines[3] = (window[0] ==  window[4]) && (window[4] == window[8]);
	line_payouts[3] = lines[3] * payout_for_symbol(window[0]);
	won_symbols[0] |= lines[3];
	won_symbols[4] |= lines[3];
	won_symbols[8] |= lines[3];

	lines[4] = (window[6] ==  window[4]) && (window[4] == window[2]);
	line_payouts[4] = lines[4] * payout_for_symbol(window[6]);
	won_symbols[6] |= lines[4];
	won_symbols[4] |= lines[4];
	won_symbols[2] |= lines[4];

	won_lines = lines[0] + lines[1] + lines[2] + lines[3] + lines[4];
	total_payout = line_payouts[0] + line_payouts[1] + line_payouts[2] +
			line_payouts[3] + line_payouts[4];
}

/*
 * Game environment.
 */

#define STATE_IDLE 0x01
#define STATE_SPIN 0x02
#define STATE_WIN 0x04
#define STATE_GAME_OVER 0x08

#define DEFAULT_BALANCE 100
#define MAX_BALANCE 1000000

uint8_t game_state = STATE_IDLE;
uint8_t do_redraw = 1;
int8_t animation_step = 0;
char* slots_status_str = "Good luck";
uint32_t balance = DEFAULT_BALANCE;
uint8_t bet = 1;

void reset_game() {
	balance = DEFAULT_BALANCE;
	bet = 1;
	slots_status_str = "Good luck";
}

void toggle_bet() {
	switch (bet) {
	case 1:
		bet = 5;
		break;
	case 5:
		bet = 10;
		break;
	default:
		bet = 1;
	}
}

/*
 * System stuff.
 */

#define PWM_TIMER_PERIOD 400
#define ENABLED_BUTTONS (BUTTON_S1 | BUTTON_S2)

#define VALIDATE_DATA(dt) (dt[0] == 0 && dt[1] > 0 && dt[1] <= MAX_BALANCE\
		&& dt[2] == 0)

char strbuf[255];

uint32_t saveable_state[3];

uint8_t style_status = OVERWRITE_TEXT;
uint8_t style_info = INVERT_TEXT | OVERWRITE_TEXT;
uint8_t style_slot = OVERWRITE_TEXT;
uint8_t style_slotwin = OVERWRITE_TEXT | INVERT_TEXT;

uint8_t slots_curr_seg = 0;

#define SEG0_ADDR   (0x1800)
#define SEG1_ADDR   (0x1880)

void save_state() {
	uint8_t status;
	uint32_t seg_write;
	uint32_t seg_erase;

	// Swap write/erase segments.
	seg_write = slots_curr_seg ? SEG1_ADDR : SEG0_ADDR;
	seg_erase = slots_curr_seg ? SEG0_ADDR : SEG1_ADDR;
	slots_curr_seg = !slots_curr_seg;

	FLASH_write32(saveable_state, (uint32_t*) (seg_write), 3);
	do {
		FLASH_segmentErase((uint8_t*) seg_erase);
		status = FLASH_eraseCheck((uint8_t*) seg_erase, 128);
	} while (status == STATUS_FAIL);
}

void read_state(uint8_t* segment, uint32_t* target) {
	target[0] = *(uint32_t*)(segment);
	target[1] = *(uint32_t*)(segment + sizeof(uint32_t));
	target[2] = *(uint32_t*)(segment + 2*sizeof(uint32_t));
}

void restore_state() {
	uint8_t status;
	// Load data from 1st segment.
	read_state((uint8_t*) SEG0_ADDR, (uint32_t*) saveable_state);
	// If something doesn't look ok - try to read from 2nd slots_curr_segment.
	if (!VALIDATE_DATA(saveable_state)) {
		read_state((uint8_t*) SEG1_ADDR, (uint32_t*) saveable_state);
	}
	// Still not ok - reset.
	if (!VALIDATE_DATA(saveable_state)) {
		saveable_state[0] = 0;
		saveable_state[1] = DEFAULT_BALANCE;
		saveable_state[2] = 0;
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
	save_state();
}

int slots_main(void)
{
	int8_t i, j;

	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	P1DIR = 0b00000011;
	P1OUT = 0x00;

	// Event loop timer: 1/2 s.
	TB0CCTL0 = CCIE;
	TB0CCR0 = 32758;
	TB0CTL = TBSSEL_1				// Input source: ACLK.
			+ MC_1
			+ TBCLR;

	// Init stuff.
	lcd_init();
	gen_slot_window();
	restore_state();
	balance = saveable_state[1];

	// Enable P2 interrupts for buttons.
	P2OUT = ENABLED_BUTTONS;
	P2DIR &= ~ENABLED_BUTTONS;
	P2REN = ENABLED_BUTTONS;
	P2SEL &= ~ENABLED_BUTTONS;
	P2IE = ENABLED_BUTTONS;
	P2IES = ENABLED_BUTTONS;
	P2IFG &= ~(ENABLED_BUTTONS);

	__bis_SR_register(GIE);

	// Main event loop.
	while (1) {

		if (do_redraw) {
			// Select text styles.
			switch (game_state) {
			case STATE_SPIN:
				style_status = OVERWRITE_TEXT | GRAYSCALE_TEXT;
				style_info = OVERWRITE_TEXT | GRAYSCALE_TEXT;
				style_slot = OVERWRITE_TEXT |
						(animation_step % 2 ? INVERT_TEXT : 0);
				break;

			case STATE_GAME_OVER:
				style_info = OVERWRITE_TEXT | GRAYSCALE_TEXT;
				style_slot = OVERWRITE_TEXT | GRAYSCALE_TEXT;
				style_status = OVERWRITE_TEXT |
						(animation_step % 2 ? INVERT_TEXT : 0);
				break;

			case STATE_WIN:
				style_status = OVERWRITE_TEXT |
						(animation_step % 2 ? INVERT_TEXT : 0);
				style_info = OVERWRITE_TEXT | GRAYSCALE_TEXT;
				style_slot = OVERWRITE_TEXT | GRAYSCALE_TEXT;
				style_slotwin = OVERWRITE_TEXT |
						(animation_step % 2 ? 0 : INVERT_TEXT);
				break;

			case STATE_IDLE:
			default:
				style_status = OVERWRITE_TEXT | GRAYSCALE_TEXT;
				style_info = INVERT_TEXT | OVERWRITE_TEXT;
				style_slot = OVERWRITE_TEXT;
				break;
			}

			// Render all the stuff.
			halLcdClearScreen();

			// Header with balance.
			sprintf(strbuf, "Balance: %d", balance);
			halLcdPrintXY(strbuf, 0, 0, style_info);

			// The slot window.
			strbuf[1] = '\0';
			for (i = 2; i >= 0; i--) {
				for (j = 2; j >= 0; j--) {
					strbuf[0] = window[j*3 + i];
					if (game_state == STATE_WIN && won_symbols[j*3 + i]) {
						halLcdPrintXY(strbuf, 35 + i*32, 20 + j*20,
								style_slotwin);
					} else {
						halLcdPrintXY(strbuf, 35 + i*32, 20 + j*20,
								style_slot);
					}
				}
			}

			// Status string and bet/spin "buttons".
			halLcdPrintXY(slots_status_str, 0, 80, style_status);
			sprintf(strbuf, "Bet: %dx5", bet);
			halLcdPrintXY(strbuf, 0, 98, style_info);
			halLcdPrintXY("SPIN", 111, 98, style_info);

			do_redraw = 0;
		}
	}
}


/*
 * Required in order to make sure that ISRs are linked only once.
 * Remove this ifdef statement if example will be used separately.
 */
#ifdef RUN_SLOTS

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_ISR(void) {
	if (game_state & STATE_IDLE) {
		// Check buttons.
		if (buttons_pressed & BUTTON_S1) {
			toggle_bet();
			do_redraw = 1;
		} else if (buttons_pressed & BUTTON_S2) {
			if (balance >= bet*5) {
				animation_step = 4;
				slots_status_str = "Spinning...";
				game_state = STATE_SPIN;
				balance -= bet * 5;
			} else {
				slots_status_str = "Low balance";
				do_redraw = 1;
			}
		}
	}
	if (game_state & STATE_SPIN) {
		// Generate next window to animate spin.
		gen_slot_window();
		do_redraw = 1;
		if (!(--animation_step)) {
			calc_line_winnings();
			if (won_lines > 0) {
				game_state = STATE_WIN;
				animation_step = 6;
				slots_status_str = " WIN WIN WIN WIN";
				balance += bet * total_payout;
				if (balance > MAX_BALANCE) {
					balance = MAX_BALANCE;
				}
			} else if (balance < 5) {
				game_state = STATE_GAME_OVER;
				slots_status_str = "    GAME OVER    ";
				animation_step = 10;
			} else {
				game_state = STATE_IDLE;
				slots_status_str = "Try again";
			}
		}
	}
	if (game_state & STATE_GAME_OVER) {
		do_redraw = 1;
		if (!(--animation_step)) {
			reset_game();
			game_state = STATE_IDLE;
		}
	}
	if (game_state & STATE_WIN) {
		do_redraw = 1;
		if (!(--animation_step)) {
			slots_status_str = "Win again";
			game_state = STATE_IDLE;
		}
	}

	// Some blinking if we're animating.
	if (animation_step && !(P1OUT & 0x03)) {
		P1OUT |= 0x01;
	} else if (animation_step) {
		P1OUT ^= 0x03;
	} else {
		P1OUT &= ~0x03;
	}

	// Save state.
	saveable_state[0] = saveable_state[2] = 0;
	saveable_state[1] = balance;
	save_state();

	// Reset buttons.
	buttons_pressed = 0;
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
	if (P2IFG & ENABLED_BUTTONS && !buttons_pressed) {
		buttons_pressed = P2IFG & ENABLED_BUTTONS;
	}
    P2IFG = 0;
}

#endif

