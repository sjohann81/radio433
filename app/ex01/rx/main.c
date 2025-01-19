#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <uart.h>
#include <printf.h>
#include <radio433.h>

int main(void){
	struct radio_data_s radiorx;
	uint8_t buf[MAX_FRAME_SIZE];
	int val, cnt = 0;
	uint8_t payload;

	uart_init(57600);
	uart_flush();
	
	printf("ok\n");
	
	radio433_setup(&radiorx, 1000, RX);

	while (1){
		/* is there any data? */
		val = radio433_rx(&radiorx, buf, &payload);
		
		/* if so, dump it to the terminal */
		if (val == ERR_OK) {
			printf("%d: (%d) ", cnt++, payload);

			for (int i = 0; i < payload; i++) {
				printf("%x ", buf[i]);
			}
			printf("\n");
		} else {
			if (val == ERR_FRAME_ERROR)
				printf("FRAME ERROR\n");
		}

		/* wait before trying again.. */
		_delay_ms(100);
	}
}
