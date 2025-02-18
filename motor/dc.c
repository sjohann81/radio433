/* file:          dc.c
 * description:   DC motor control
 * version:       v0.01
 * date:          01/2025
 * author:        Sergio Johann Filho <sergiojohannfilho@gmail.com>
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <dc.h>

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/* init PWM for DC motor control
 */
void dc_init(uint8_t speed)
{
#ifdef ALT_DC_CONFIG
	/* set timer 0 for phase correct PWM mode */
	TCCR0A = (1 << WGM00);
	/* prescaler is 64, PWM @ 490Hz */
	TCCR0B = (1 << CS01) | (1 << CS00);
	
	/* duty cycle: 0% */
	OCR0A = 0;
	OCR0B = 0;
#else
	/* configure timer1 for phase correct PWM mode with TOP value */
	TCCR1A = (1 << WGM11);

	switch (speed) {
	case PWM_25:
		/* prescaler is 8 for PWM @ 25Hz, 50Hz, 100Hz or 250Hz */
		TCCR1B = (1 << WGM13) | (1 << CS11);
		ICR1 = PWM_25_MAX + 1;
		break;
	case PWM_50:
		TCCR1B = (1 << WGM13) | (1 << CS11);
		ICR1 = PWM_50_MAX + 1;
		break;
	case PWM_100:
		TCCR1B = (1 << WGM13) | (1 << CS11);
		ICR1 = PWM_100_MAX + 1;
		break;
	case PWM_250:
		TCCR1B = (1 << WGM13) | (1 << CS11);
		ICR1 = PWM_250_MAX + 1;
		break;
	case PWM_500:
		/* prescaler is 1 for PWM >= 500Hz */
		TCCR1B = (1 << WGM13) | (1 << CS10);
		ICR1 = PWM_500_MAX + 1;
		break;
	case PWM_1k:
		TCCR1B = (1 << WGM13) | (1 << CS10);
		ICR1 = PWM_1k_MAX + 1;
		break;
	case PWM_2k5:
		TCCR1B = (1 << WGM13) | (1 << CS10);
		ICR1 = PWM_2k5_MAX + 1;
		break;
	case PWM_5k:
		TCCR1B = (1 << WGM13) | (1 << CS10);
		ICR1 = PWM_5k_MAX + 1;
		break;
	case PWM_10k:
		TCCR1B = (1 << WGM13) | (1 << CS10);
		ICR1 = PWM_10k_MAX + 1;
		break;
	case PWM_20k:
		TCCR1B = (1 << WGM13) | (1 << CS10);
		ICR1 = PWM_500_MAX + 1;
		break;
	}
	
	/* duty cycle: 0% */
	OCR1A = 0;
	OCR1B = 0;
#endif
}

int dc_attach(uint8_t channel)
{
	switch (channel) {
	case 1:
		/* enable PWM output on OC0A */
		dc_direction(1, STOP);
		DC_DIR |= (1 << DC_CTRL1A) | (1 << DC_CTRL2A) | (1 << DC_PWMA);
#ifdef ALT_DC_CONFIG
		TCCR0A |= (1 << COM0A1);
#else
		TCCR1A |= (1 << COM1A1);
#endif
		break;
	case 2:
		/* enable PWM output on OC0B */
		dc_direction(2, STOP);
		DC_DIR |= (1 << DC_CTRL1B) | (1 << DC_CTRL2B) | (1 << DC_PWMB);
#ifdef ALT_DC_CONFIG
		TCCR0A |= (1 << COM0B1);
#else
		TCCR1A |= (1 << COM1B1);
#endif
		break;
	default:
		return -1;
	}
	
	return 0;
}

int dc_dettach(uint8_t channel)
{
	switch (channel) {
	case 1:
		/* disable PWM output on OC0A */
		dc_direction(1, STOP);
		DC_DIR &= ~((1 << DC_CTRL1A) | (1 << DC_CTRL2A) | (1 << DC_PWMA));
#ifdef ALT_DC_CONFIG
		TCCR0A &= ~(1 << COM0A1);
#else
		TCCR1A &= ~(1 << COM1A1);
#endif
		break;
	case 2:
		/* disable PWM output on OC0B */
		dc_direction(2, STOP);
		DC_DIR &= ~((1 << DC_CTRL1B) | (1 << DC_CTRL2B) | (1 << DC_PWMB));
#ifdef ALT_DC_CONFIG
		TCCR0A &= ~(1 << COM0B1);
#else
		TCCR1A &= ~(1 << COM1B1);
#endif
		break;
	default:
		return -1;
	}
	
	return 0;
}

int dc_direction(uint8_t channel, uint8_t direction)
{
	switch (channel) {
	case 1:
		if (direction == STOP)
			DC_PORT &= ~((1 << DC_CTRL1A) | (1 << DC_CTRL2A));
		if (direction == FORWARD) {
			DC_PORT &= ~(1 << DC_CTRL2A);
			DC_PORT |= (1 << DC_CTRL1A);
		}
		if (direction == REVERSE) {
			DC_PORT &= ~(1 << DC_CTRL1A);
			DC_PORT |= (1 << DC_CTRL2A);
		}
		break;
	case 2:
		if (direction == STOP)
			DC_PORT &= ~((1 << DC_CTRL1B) | (1 << DC_CTRL2B));
		if (direction == FORWARD) {
			DC_PORT &= ~(1 << DC_CTRL2B);
			DC_PORT |= (1 << DC_CTRL1B);
		}
		if (direction == REVERSE) {
			DC_PORT &= ~(1 << DC_CTRL1B);
			DC_PORT |= (1 << DC_CTRL2B);
		}
		break;
	default:
		return -1;
	}
	
	return 0;
}

/* speed is a value between 0 and 255 (timer0) or 0 and 15999 (timer1) */
int dc_write(uint8_t channel, uint16_t speed)
{
	switch (channel) {
	case 1:
#ifdef ALT_DC_CONFIG
		OCR0A = speed & 0xff;
#else
		OCR1A = speed;
#endif
		break;
	case 2:
#ifdef ALT_DC_CONFIG
		OCR0B = speed & 0xff;
#else
		OCR1B = speed;
#endif
		break;
	default:
		return -1;
	}
	
	return 0;
}
