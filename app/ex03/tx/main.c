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
	struct radio_data_s radiotx;
	uint8_t data[sizeof(struct appdata_s)];
	struct appdata_s *const control = (struct appdata_s *)data;

	uart_init(57600);
	uart_flush();

	printf("ok\n");
	
	radio433_setup(&radiotx, 2000, TX);
	radio433_addr(&radiotx, 0x1234);
	
	memset(data, 0, sizeof(data));

	while (1){
		/* simulate changes in channel values */
		control->ch1++;
		control->ch2--;
		control->ch3 += 2;
		control->ch4 -= 2;
		control->ch5 += 3;
		control->ch6 -= 3;
		
		/* send a control message */
		radio433_send(&radiotx, 0x1234, data, sizeof(struct appdata_s));
		
		/* we are sending ~5 packets/s @ 2000bps*/
		_delay_ms(200);
	}
}
