#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <uart.h>
#include <printf.h>
#include <radio433.h>
#include <dc.h>

#define RADIO_RATE		1000
#define RADIO_TIMEOUT		100
//#define DEBUG

struct appdata_s {
	int8_t ch1;		// steering (analog steering emulation)
	int8_t ch2;		// throttle (analog throttle emulation)
	int8_t ch3;		// switches (throttle and steering dual 
				// rates, acceleration mode, lights, horn ...)
	int8_t ch4;		// not used
	uint8_t sum;		// simple checksum
};


uint8_t chksum(uint8_t *data, uint16_t size)
{
	uint16_t sum = 0;
	
	for (int i = 0; i < size; i++)
		sum += data[i];
	
	return ~(sum & 0xff);
}

void init_ports()
{
	/* disable input pin interrupts */
	PCMSK2 = 0;
	PCICR = 0;
}

int main(void){
	struct radio_data_s radiorx;
	uint8_t data[MAX_DATA_SIZE];
	struct appdata_s *const control = (struct appdata_s *)data;
	int val;
	uint8_t payload;
	int16_t dc1, dc2;
	uint16_t timeout = 0;

	/* wait, so the MCU can be reprogrammed after a reset */
	/* press reset and in less than 2s try to flash it */
	_delay_ms(2000);

	init_ports();
	
	uart_init(57600);
	uart_flush();

	dc_init(PWM_25);
	dc_attach(1);
	dc_attach(2);
	dc_direction(1, STOP);
	dc_direction(2, STOP);

	radio433_setup(&radiorx, RADIO_RATE, RX);

	while (1){
		/* is there any data? */
		val = radio433_rx(&radiorx, data, &payload);

		/* is it ok? */
		if (val == ERR_OK) {
#ifdef DEBUG
			printf("(%d bytes) --> ch1: %d ch2: %d ch3: %d ch4: %d sum: %d\n",
				payload, control->ch1, control->ch2,
				control->ch3, control->ch4, control->sum);
#endif
			if (chksum(data, sizeof(struct appdata_s) - sizeof(uint8_t)) != control->sum) {
#ifdef DEBUG
				printf("CRC ERROR\n");
#endif
				continue;
			}

			if (control->ch3 & 0x01) {
				dc_direction(1, FORWARD);
				dc_direction(2, FORWARD);
			} else if (control->ch3 & 0x02) {
				dc_direction(1, REVERSE);
				dc_direction(2, REVERSE);
			} else {
				dc_direction(1, STOP);
				dc_direction(2, STOP);
			}
			
			if (control->ch1 < 0) {
				dc1 = control->ch2 + 127;
				dc2 = map(control->ch1, -127, 0, 0, dc1);
			} else {
				dc2 = control->ch2 + 127;
				dc1 = map(control->ch1, 0, 127, dc2, 0);
			}
#ifdef DEBUG
			printf("DC1: %d DC2: %d\n", dc1, dc2);
#endif
			dc_write(1, map(dc1, 0, 255, 0, PWM_25_MAX));
			dc_write(2, map(dc2, 0, 255, 0, PWM_25_MAX));
			
			timeout = 0;
		} else {
			if (timeout++ > RADIO_TIMEOUT) {
				dc_direction(1, STOP);
				dc_direction(2, STOP);
			}
		}

		_delay_ms(10);
	}
}
