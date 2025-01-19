#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <uart.h>
#include <printf.h>
#include <radio433.h>

int main(void){
	struct radio_data_s radiotx;
	uint8_t buf[MAX_DATA_SIZE];
	uint8_t buf2[MAX_DATA_SIZE];
	uint8_t cnt = 0;

	memset(buf, 0x55, MAX_DATA_SIZE);
	buf[0] = 0x80;
	buf[1] = 0x01;
	buf[2] = 0xaa;
	buf[3] = 0x00;
	buf[4] = 0x01;
	buf[31] = 0x69;

	uart_init(57600);
	uart_flush();

	printf("ok\n");
	
	radio433_setup(&radiotx, 1000, TX);
	radio433_addr(&radiotx, 0x5151);

	while (1){
		/* send a short message */
		radio433_send(&radiotx, 0x5150, buf, 5);
		_delay_ms(500);
		
		/* another message (broadcast) */
		for (int i = 0; i < 16; i++)
			buf2[i] = cnt++;
		radio433_send(&radiotx, BCAST_ADDR, buf2, 16);
		_delay_ms(1500);
		
		/* send a large message */
		radio433_send(&radiotx, 0x5150, buf, MAX_DATA_SIZE);
		_delay_ms(2000);
	}
}
