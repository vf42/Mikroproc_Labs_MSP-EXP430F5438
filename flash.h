/*
 * flash.h
 *
 *  Created on: 10 Dec 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */

#ifndef FLASH_H_
#define FLASH_H_

#include <stdint.h>

extern void read_time(uint8_t* segment, uint8_t* target);
extern void load_and_reset();
extern void save_time(uint8_t* data);

extern int flash_main(void);

#endif /* FLASH_H_ */
