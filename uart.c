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

/*
 * A simple uart driver for the RPi.
 */

#include "system.h"
#include "uart.h"

/*
 * The primary serial console that we end up using is the normal UART, not
 * the mini-uart that shares interrupts and registers with the SPI masters
 * as well.
 */

#define	UART_BASE		(mmio_base + 0x00201000)
#define	UART_DR			0x0
#define	UART_FR			0x18
#define	UART_IBRD		0x24
#define	UART_FBRD		0x28
#define	UART_LCRH		0x2c
#define	UART_CR			0x30
#define	UART_ICR		0x44

#define	UART_DR_DATA		0x0ff
#define	UART_DR_FE		0x100
#define	UART_DR_PE		0x200
#define	UART_DR_BE		0x400
#define	UART_DR_OE		0x800

#define	UART_FR_RXFE		0x10		/* RX fifo empty */
#define	UART_FR_TXFF		0x20		/* TX fifo full */

#define	UART_LCRH_FEN		0x00000010	/* fifo enable */
#define	UART_LCRH_WLEN_8	0x00000060	/* 8 bits */

#define	UART_CR_UARTEN		0x001		/* uart enable */
#define	UART_CR_TXE		0x100		/* TX enable */
#define	UART_CR_RXE		0x200		/* RX enable */

#define UART_DEFAULT_CLK	3000000
#define UART_RATE		115200


/*
 * All we care about are pins 14 and 15 for the UART.  Specifically, alt0
 * for GPIO14 is TXD0 and GPIO15 is RXD0. Those are controlled by FSEL1.
 */
#define	GPIO_BASE	(mmio_base + 0x00200000)
#define	GPIO_FSEL1	0x4
#define	GPIO_PUD	0x94
#define	GPIO_PUDCLK0	0x98

#define GPIO_SEL_ALT0	0x4
#define	GPIO_UART_MASK	0xfffc0fff
#define	GPIO_UART_TX_SHIFT	12
#define	GPIO_UART_RX_SHIFT	15

#define	GPIO_PUD_DISABLE	0x0
#define	GPIO_PUDCLK_UART	0x0000c000

/*
 * Statistics
 */
struct uart_stats uart_stats;

/*
 * A simple nop
 */
static void
uart_nop(void)
{
	__asm__ volatile("mov r0, r0\n" : : :);
}

/*
 * MMIO register access
 */
static inline void
reg_write(uint32_t addr, uint32_t val)
{
	volatile uint32_t *ptr = (volatile uint32_t *)addr;

	*ptr = val;
}

static inline uint32_t
reg_read(uint32_t addr)
{
	volatile uint32_t *ptr = (volatile uint32_t *)addr;

	return *ptr;
}

/*
 * uartclk / (16 * bps) = rate
 * IBRD = (int) rate
 * FBRD = (int) (fraction(rate) * 64 + 0.5)
 *
 * To avoid floating point math, we pretend that the clock runs 128 times
 * faster.  This will yield a rate that's 128x.  For the integer part, we
 * can simply divide by 128.  For the floating point part, we subtract away
 * the integer part worth, add one (this is 1/128 of the actual rate, which
 * is 0.5 of 1/64), and then divide by 2.
 *
 * It is safe for us to mulitply the rate by 128 since all clock frequencies
 * up to 384 MHz fit into a 32-bit register and the default is 3MHz.
 */
static void
uart_set_baud_rate(uint32_t clk)
{
	uint32_t rate;
	uint32_t i, f;

	if (!clk)
		clk = UART_DEFAULT_CLK;

	clk *= 128;

	rate = clk / (16 * UART_RATE);
	i = rate / 128;
	f = ((rate - i * 128) + 1) / 2;

	reg_write(UART_BASE + UART_IBRD, i);
	reg_write(UART_BASE + UART_FBRD, f);
}

void
uart_init(uint32_t uart_clock)
{
	uint32_t v;
	int i;

	/* disable UART */
	reg_write(UART_BASE + UART_CR, 0);

	/* TODO: Factor out the gpio bits */
	v = reg_read(GPIO_BASE + GPIO_FSEL1);
	v &= GPIO_UART_MASK;
	v |= GPIO_SEL_ALT0 << GPIO_UART_RX_SHIFT;
	v |= GPIO_SEL_ALT0 << GPIO_UART_TX_SHIFT;
	reg_write(GPIO_BASE + GPIO_FSEL1, v);

	reg_write(GPIO_BASE + GPIO_PUD, GPIO_PUD_DISABLE);
	for (i = 0; i < 150; i++)
		uart_nop();
	reg_write(GPIO_BASE + GPIO_PUDCLK0, GPIO_PUDCLK_UART);
	for (i = 0; i < 150; i++)
		uart_nop();
	reg_write(GPIO_BASE + GPIO_PUDCLK0, 0);

	/* clear all interrupts */
	reg_write(UART_BASE + UART_ICR, 0x7ff);

	/* set the baud rate */
	uart_set_baud_rate(uart_clock);

	/* select 8-bit, enable FIFOs */
	reg_write(UART_BASE + UART_LCRH, UART_LCRH_WLEN_8 | UART_LCRH_FEN);

	/* enable UART */
	reg_write(UART_BASE + UART_CR, UART_CR_UARTEN | UART_CR_TXE |
	    UART_CR_RXE);
}

void
uart_putc(uint8_t c)
{
	uart_putbyte(c & 0x7f);
	if (c == '\n')
		uart_putbyte('\r');
}

void
uart_putbyte(uint8_t c)
{
	while (reg_read(UART_BASE + UART_FR) & UART_FR_TXFF)
		;
	reg_write(UART_BASE + UART_DR, c);
	uart_stats.tx_bytes++;
}

uint8_t
uart_getc(void)
{
	return (reg_read(UART_BASE + UART_DR) & 0x7f);
}

uint8_t
uart_getbyte(void)
{
	uint32_t byte;

	while (reg_read(UART_BASE + UART_FR) & UART_FR_RXFE)
		;
	byte = reg_read(UART_BASE + UART_DR);

	uart_stats.rx_bytes++;
	if (byte & UART_DR_FE)
		uart_stats.framing_error++;
	if (byte & UART_DR_PE)
		uart_stats.parity_error++;
	if (byte & UART_DR_BE)
		uart_stats.break_error++;
	if (byte & UART_DR_OE)
		uart_stats.overrun_error++;

	return byte & UART_DR_DATA;
}

int
uart_isc(void)
{
	return ((reg_read(UART_BASE + UART_FR) & UART_FR_RXFE) == 0);
}
