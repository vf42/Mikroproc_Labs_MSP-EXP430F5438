/*
 * common.h
 *
 *  Created on: 11 Dec 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */

#ifndef COMMON_H_
#define COMMON_H_

#define INVALID_TIME(td) (td[0] >= 60 || td[1] >= 60 || td[2] >= 24)
#define VALIDATE_DIGIT(d) (d >= '0' && d <= '9')

extern uint8_t time_data[3];
extern uint8_t buttons_pressed;

extern int parse_time(char* str);

extern void lcd_init();

#endif /* COMMON_H_ */
