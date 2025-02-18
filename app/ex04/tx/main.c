#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <uart.h>
#include <printf.h>
#include <radio433.h>
#include <adc.h>
#include <dc.h>

#define RADIO_RATE		1000

#define CTRL_DIR		DDRD
#define CTRL_PORT		PORTD
#define CTRL_PIN		PIND
#define CTRL_PSW1		(1 << PD2)
#define CTRL_PSW2		(1 << PD3)
#define CTRL_BACKWARD		(1 << PD4)
#define CTRL_FORWARD		(1 << PD5)
#define CTRL_SW1		(1 << PD6)
#define CTRL_SW2		(1 << PD7)
#define IDLE_MS			150		// idle time in ms
#define STEER_LIMIT		127		// + and - steer limit
#define ACCEL_LIMIT		127		// + and - accel limit


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

int8_t read_steering()
{
	uint16_t val;
	
	adc_set_channel(0);
	val = adc_read();
	
	return map(val, 0, 1023, -STEER_LIMIT, STEER_LIMIT);
}

int8_t read_throttle()
{
	uint16_t val;
	
	adc_set_channel(1);
	val = adc_read();
	
	return map(val, 0, 1023, -ACCEL_LIMIT, ACCEL_LIMIT);
}

int8_t read_switches()
{
	int8_t val = 0;
	
	if (!(CTRL_PIN & CTRL_FORWARD))
		val |= 0x01;
		
	if (!(CTRL_PIN & CTRL_BACKWARD))
		val |= 0x02;
	
	if (!(CTRL_PIN & CTRL_SW1))
		val |= 0x04;
		
	if (!(CTRL_PIN & CTRL_SW2))
		val |= 0x08;
		
	return val;
}

void init_ports()
{
	/* switches are inputs */
	CTRL_DIR &= ~(CTRL_FORWARD | CTRL_BACKWARD | CTRL_SW1 | CTRL_SW2);
	
	/* enable internal pull-ups, switches are active low */
	CTRL_PORT |= CTRL_FORWARD | CTRL_BACKWARD | CTRL_SW1 | CTRL_SW2;
	
	/* disable input pin interrupts */
	PCMSK2 = 0;
	PCICR = 0;
}

int main(void)
{
	struct radio_data_s radiotx;
	uint8_t data[sizeof(struct appdata_s)];
	struct appdata_s *const control = (struct appdata_s *)data;
	
	/* wait, so the MCU can be reprogrammed after a reset */
	/* press reset and in less than 2s try to flash it */
	_delay_ms(2000);

	init_ports();
	adc_init();
	
	radio433_setup(&radiotx, RADIO_RATE, TX);
	
	memset(data, 0, sizeof(data));
	control->ch4 = 0;
	
	while (1) {
		/* read switches */
		control->ch1 = read_steering();
		control->ch2 = read_throttle();
		control->ch3 = read_switches();
		control->ch4++;
		control->sum = chksum(data, sizeof(struct appdata_s) - sizeof(uint8_t));
		
		/* send a control message */
		radio433_tx(&radiotx, data, sizeof(struct appdata_s));
		
		/* we are sending < 5 packets/s @ 1000bps*/
		_delay_ms(IDLE_MS);
	}
}
