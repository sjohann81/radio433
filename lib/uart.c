/* file:          uart.c
 * description:   interrupt driven UART driver
 * version:       v0.01
 * date:          01/2025
 * author:        Sergio Johann Filho <sergiojohannfilho@gmail.com>
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "uart.h"


#define RX_BUFFER_SIZE		32
#define RX_BUFFER_MASK		(RX_BUFFER_SIZE - 1)

struct uart_s {
	volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
	volatile uint16_t rx_head, rx_tail, rx_size;
	volatile uint32_t rx_errors;
};

static struct uart_s uart;
static struct uart_s *uart_p = &uart;

void uart_init(uint32_t baud)
{
	uint32_t ubrr_val;
	
	cli();
	/* set USART baud rate */
	ubrr_val = ((uint32_t)F_CPU / (16 * baud)) - 1;
	UBRR0H = (uint8_t)(ubrr_val >> 8);
	UBRR0L = (uint8_t)(ubrr_val & 0xff);
	/* set frame format to 8 data bits, no parity, 1 stop bit */
	UCSR0C = (0 << USBS0) | (3 << UCSZ00);
	/* enable receiver, transmitter and receiver interrupt */
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
	sei();
}

void uart_flush(void)
{
	uart_p->rx_head = 0;
	uart_p->rx_tail = 0;
	uart_p->rx_size = 0;
}

uint16_t uart_rxsize(void)
{
	return uart_p->rx_size;
}

void uart_tx(uint8_t data)
{
	/* wait if a byte is being transmitted */
	while ((UCSR0A & (1 << UDRE0)) == 0);
		
	/* transmit data */
	UDR0 = data;
}

uint8_t uart_rx(void)
{
	uint8_t data;
	
	if (uart_p->rx_head == uart_p->rx_tail)
		return 0;

	cli();
	data = uart_p->rx_buffer[uart_p->rx_head];
	uart_p->rx_head = (uart_p->rx_head + 1) & RX_BUFFER_MASK;
	uart_p->rx_size--;
	sei();
	
	return data;
}

ISR(USART_RX_vect)
{
	uint16_t tail;

	while ((UCSR0A & (1 << RXC0)) != 0) {
		// if there is space, put data in rx fifo
		tail = (uart_p->rx_tail + 1) & RX_BUFFER_MASK;
		if (tail != uart_p->rx_head) {
			uart_p->rx_buffer[uart_p->rx_tail] = UDR0;
			uart_p->rx_tail = tail;
			uart_p->rx_size++;
		} else {
			uart_p->rx_errors++;
		}
	}
}
