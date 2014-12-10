/*
 * main.c
 *
 * Common entry point that allows to run several examples.
 *
 *  Created on: 18 Sep 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */
#include "msp430.h"

#include "select.h"
#include "blink.h"
#include "timers.h"
#include "pwm.h"
#include "flash.h"
#include "uart.h"
#include "sound_meter.h"
#include "clock.h"
#include "slots.h"

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	
    // !! See select.h !!
#ifdef RUN_BLINK
    blink_main();
#endif
#ifdef RUN_TIMERS
    timers_main();
#endif
#ifdef RUN_PWM
    pwm_main();
#endif
#ifdef RUN_FLASH
    flash_main();
#endif
#ifdef RUN_UART
    uart_main();
#endif
#ifdef RUN_SOUND_METER
    sound_meter_main();
#endif
#ifdef RUN_CLOCK
    clock_main();
#endif
#ifdef RUN_SLOTS
    slots_main();
#endif

	return 0;
}
