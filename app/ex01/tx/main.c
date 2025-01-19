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
	uint8_t buf[MAX_FRAME_SIZE];
	uint8_t buf2[MAX_FRAME_SIZE];
	uint8_t cnt = 0;

	memset(buf, 0x55, MAX_FRAME_SIZE);
	buf[0] = 0x80;
	buf[1] = 0x01;
	buf[2] = 0xaa;
	buf[3] = 0x00;
	buf[4] = 0x01;
	buf[39] = 0x69;
	
	uart_init(57600);
	uart_flush();

	printf("ok\n");
	
	radio433_setup(&radiotx, 1000, TX);

	while (1){
		/* send a short message */
		radio433_tx(&radiotx, buf, 5);
		_delay_ms(200);
		
		/* another message */
		for (int i = 0; i < 16; i++)
			buf2[i] = cnt++;
		radio433_tx(&radiotx, buf2, 16);
		_delay_ms(1000);
		
		/* send a large message */
		radio433_tx(&radiotx, buf, MAX_FRAME_SIZE);
		_delay_ms(1000);

	}
}
