/*
 * uart.h
 *
 *  Created on: 10 Dec 2014
 *      Author: Vadim Fedorov <vadim.fedorov@gmail.com>
 */

#ifndef UART_H_
#define UART_H_

extern char rcvbuff[255];
extern unsigned char receiveBufferSize;

extern char cmdbuff[255];
extern unsigned char cmdbuff_size;

extern void clear_uart_buffer();
extern void uart_init(void);
extern void uart_send_char(char character);
extern void uart_send_str(char* string, int length);

extern int uart_main(void);

#endif /* UART_H_ */
