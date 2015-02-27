/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright 2013 (c) Joyent, Inc.  All rights reserved.
 * Copyright 2015 (c) Josef 'Jeff' Sipek <jeffpc@josefsipek.net>
 */

#ifndef __UART_H
#define	__UART_H

/*
 * Interface to the a PL011 uart.
 */

#ifdef __cplusplus
extern "C" {
#endif

void uart_init(void);
void uart_putc(uint8_t);
void uart_putbyte(uint8_t);
uint8_t uart_getc(void);
uint8_t uart_getbyte(void);
int uart_isc(void);

/* some useful things exposed for others to use */
extern const char * const platform_name;

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */
