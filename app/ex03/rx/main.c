#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <uart.h>
#include <printf.h>
#include <radio433.h>

struct appdata_s {
	int8_t ch1;
	int8_t ch2;
	int8_t ch3;
	int8_t ch4;
	int8_t ch5;
	int8_t ch6;
};

int main(void){
	struct radio_data_s radiorx;
	uint8_t data[MAX_DATA_SIZE];
	struct appdata_s *const control = (struct appdata_s *)data;
	int val, cnt = 0;
	uint8_t payload;
	uint16_t src_addr;

	uart_init(57600);
	uart_flush();
	
	printf("ok\n");
	
	radio433_setup(&radiorx, 1000, RX);
	radio433_addr(&radiorx, 0x1234);
	
	while (1){
		/* is there any data? */
		val = radio433_recv(&radiorx, &src_addr, data, &payload);
		
		/* if so, dump it to the terminal */
		if (val == ERR_OK) {
			printf("%d: (%d bytes from %x) --> ch1: %d ch2: %d ch3: %d ch4: %d ch5: %d ch6: %d\n",
			 cnt++, payload, src_addr, control->ch1, control->ch2,
			 control->ch3, control->ch4, control->ch5, control->ch6);
		} else {
			if (val == ERR_FRAME_ERROR)
				printf("FRAME ERROR\n");
			if (val == ERR_CRC_ERROR)
				printf("CRC ERROR\n");
		}
	}
}
