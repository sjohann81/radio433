#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <uart.h>
#include <printf.h>
#include <radio433.h>

#define RADIO_RATE		1000
#define RADIO_ADDR		0x07bd

#define CTRL_DIR		DDRD
#define CTRL_PORT		PORTD
#define CTRL_PIN		PIND
#define CTRL_LEFT		(1 << PD2)
#define CTRL_RIGHT		(1 << PD3)
#define CTRL_BACKWARD		(1 << PD4)
#define CTRL_FORWARD		(1 << PD5)
#define CTRL_SW1		(1 << PD6)	// drift mode
#define CTRL_SW2		(1 << PD7)	// low throttle mode
#define CTRL_IDLE_MS		300		// idle time in ms
#define CTRL_STEER_LIMIT	127		// + and - steer limit
#define CTRL_ACCEL_LIMIT	127		// + and - accel limit
#define CTRL_STEER_RATE		15
#define CTRL_ACCEL_RATE		15


struct appdata_s {
	int8_t ch1;		// steering (analog steering emulation)
	int8_t ch2;		// throttle (analog throttle emulation)
	int8_t ch3;		// switches (throttle and steering dual 
				// rates, acceleration mode, lights, horn ...)
	int8_t ch4;		// raw steering and throttle switches
};


int8_t read_steering()
{
	static int16_t val = 0;
	
	if ((CTRL_PIN & CTRL_LEFT) && (CTRL_PIN & CTRL_RIGHT)) {
		if (val < 0) {
			val += CTRL_STEER_RATE * 3;
			if (val > 0) val = 0;
		} else {
			val -= CTRL_STEER_RATE * 3;
			if (val < 0) val = 0;
		}
	} else {
		if (!(CTRL_PIN & CTRL_LEFT))
			val -= CTRL_STEER_RATE;
		
		if (!(CTRL_PIN & CTRL_RIGHT))
			val += CTRL_STEER_RATE;
		
		if (val < -CTRL_STEER_LIMIT) val = -CTRL_STEER_LIMIT;
		if (val > CTRL_STEER_LIMIT) val = CTRL_STEER_LIMIT;
	}
	
	return val;
}

int8_t read_throttle()
{
	static int16_t val = 0;
	
	if ((CTRL_PIN & CTRL_BACKWARD) && (CTRL_PIN & CTRL_FORWARD)) {
		if (val < 0) {
			val += CTRL_ACCEL_RATE * 3;
			if (val > 0) val = 0;
		} else {
			val -= CTRL_ACCEL_RATE * 3;
			if (val < 0) val = 0;
		}
	} else {
		if (!(CTRL_PIN & CTRL_BACKWARD))
			val -= CTRL_ACCEL_RATE;
		
		if (!(CTRL_PIN & CTRL_FORWARD))
			val += CTRL_ACCEL_RATE;
		
		if (val < -CTRL_ACCEL_LIMIT) val = -CTRL_ACCEL_LIMIT;
		if (val > CTRL_ACCEL_LIMIT) val = CTRL_ACCEL_LIMIT;
	}
	
	return val;
}

int8_t read_switches()
{
	int8_t val = 0;
	
	if (!(CTRL_PIN & CTRL_SW1))
		val |= 0x01;
		
	if (!(CTRL_PIN & CTRL_SW2))
		val |= 0x02;
		
	return val;
}

int8_t read_raw()
{
	int8_t val = 0;
	
	if (!(CTRL_PIN & CTRL_LEFT))
		val |= 0x01;
	
	if (!(CTRL_PIN & CTRL_RIGHT))
		val |= 0x02;

	if (!(CTRL_PIN & CTRL_BACKWARD))
		val |= 0x04;
		
	if (!(CTRL_PIN & CTRL_FORWARD))
		val |= 0x08;
		
	return val;
}

void init_ports()
{
	/* switches are inputs */
	CTRL_DIR &= ~(CTRL_LEFT | CTRL_RIGHT | CTRL_FORWARD | CTRL_BACKWARD);
	CTRL_DIR &= ~(CTRL_SW1 | CTRL_SW2);
	
	/* enable internal pull-ups, switches are active low */
	CTRL_PORT |= CTRL_LEFT | CTRL_RIGHT | CTRL_FORWARD | CTRL_BACKWARD;
	CTRL_PORT |= CTRL_SW1 | CTRL_SW2;
	
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
	radio433_setup(&radiotx, RADIO_RATE, TX);
	radio433_addr(&radiotx, RADIO_ADDR);
	
	memset(data, 0, sizeof(data));
	
	while (1) {
		/* read switches */
		control->ch1 = read_steering();
		control->ch2 = read_throttle();
		control->ch3 = read_switches();
		control->ch4 = read_raw();
		
		/* no need to send data */
//		if (!control->ch1 && !control->ch2 && !control->ch4) continue;
		
		/* send a control message */
		radio433_send(&radiotx, RADIO_ADDR, data, sizeof(struct appdata_s));
		
		/* we are sending < 5 packets/s @ 1000bps*/
		_delay_ms(CTRL_IDLE_MS);
	}
}
