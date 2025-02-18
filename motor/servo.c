/* file:          servo.c
 * description:   servo motor control and ESC driver
 * version:       v0.01
 * date:          01/2025
 * author:        Sergio Johann Filho <sergiojohannfilho@gmail.com>
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <servo.h>


/* servo / ESC control using timer0, 8 channels */

static struct servo_t servos[MAX_CHANNELS + 1];
static volatile uint8_t channel = 0;
static volatile uint8_t isr_count = 0;
static uint8_t channels = 0;

ISR(TIMER0_OVF_vect)
{
	if (++isr_count == servos[channel].count) {
		TCNT0 = servos[channel].countdown;
		
		return;
	}

	if (isr_count > servos[channel].count) {
		if (servos[channel].enabled)
			SERVO_PORT &= ~(1 << servos[channel].pin);
			
		channel++;
		isr_count = 0;
		TCNT0 = 0;
		
		if ((channel != 0) && (channel <= MAX_CHANNELS)) {
			if (servos[channel].enabled)
				SERVO_PORT |= (1 << servos[channel].pin);
		} else {
			if (channel > MAX_CHANNELS)
				channel = 0;
		}
	}
}

void servo0_init()
{
/* timer0 in normal mode, prescaler is 8 */
#ifndef ATMEGA8
	TCCR0A = 0;
	TCCR0B = (1 << CS01);
	TCNT0 = 0;
	TIMSK0 = (1 << TOIE0);
#else
	TCCR0 = (1 << CS01);
	TCNT0 = 0;
	TIMSK = (1 << TOIE0);
#endif
	sei();
}

int servo0_attach(uint8_t channel, uint8_t pin)
{
	if (channel > 0 && channel <= MAX_CHANNELS) {
		channels++;
		servos[0].count = ((SYNC_PERIOD - ((channels + 1) * DEFAULT_PULSE_WIDTH)) >> 7);
		servo0_write(channel, DEFAULT_PULSE_WIDTH);
		SERVO_DIR |= (1 << pin);
		SERVO_PORT &= ~(1 << pin);
		servos[channel].pin = pin;
		servos[channel].enabled = 1;
	
		return 0;
	}
	
	return -1;
}

int servo0_dettach(uint8_t channel)
{
	if (channel > 0 && channel <= MAX_CHANNELS) {
		channels--;
		servos[0].count = ((SYNC_PERIOD - ((channels + 1) * DEFAULT_PULSE_WIDTH)) >> 7);
		SERVO_DIR &= ~(1 << servos[channel].pin);
		servos[channel].enabled = 0;
				
		return 0;
	}
	
	return -1;
}

/* pulse width is in microseconds (1000 steps) */
int servo0_write(uint8_t channel, uint16_t pulsewidth)
{
	if (channel > 0 && channel <= MAX_CHANNELS) {
		if (pulsewidth < MIN_PULSE_WIDTH)
			pulsewidth = MIN_PULSE_WIDTH;
		if (pulsewidth > MAX_PULSE_WIDTH)
			pulsewidth = MAX_PULSE_WIDTH;	 

		servos[channel].count = pulsewidth >> 7;	
		servos[channel].countdown = 255 - ((pulsewidth - (servos[channel].count << 7)) << 1);

		return 0;
	}
	
	return -1;
}


/* servo / ESC control using timer1, 2 channels.
 * fine control for ESC, but pins are fixed to PB1 and PB2
 * 
 * timer1 is shared with DC motor control!
 */

void servo1_init()
{
	/* configure timer1 for fast PWM mode with TOP value */
	TCCR1A = (1 << WGM11);
	/* prescaler is 8, PWM @ 50Hz */
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
	ICR1 = 40000;
	
	/* Set initial duty cycle to 1.5ms on both channels */
	OCR1A = DEFAULT_PULSE_WIDTH << 1;
	OCR1B = DEFAULT_PULSE_WIDTH << 1;
}

int servo1_attach(uint8_t channel)
{
	switch (channel) {
	case 1:
		/* enable PWM output on OC1A */
		DDRB |= (1 << PB1);
		TCCR1A |= (1 << COM1A1);
		break;
	case 2:
		/* enable PWM output on OC1B */
		DDRB |= (1 << PB2);
		TCCR1A |= (1 << COM1B1);
		break;
	default:
		return -1;
	}
	
	return 0;
}

int servo1_dettach(uint8_t channel)
{
	switch (channel) {
	case 1:
		/* disable PWM output on OC1A */
		DDRB &= ~(1 << PB1);
		TCCR1A &= ~(1 << COM1A1);
		break;
	case 2:
		/* disable PWM output on OC1B */
		DDRB &= ~(1 << PB2);
		TCCR1A &= ~(1 << COM1B1);
		break;
	default:
		return -1;
	}
	
	return 0;
}

/* pulse width is in microseconds times two (2000 steps) */
int servo1_write(uint8_t channel, uint16_t pulsewidth)
{
	if (pulsewidth < MIN_PULSE_WIDTH << 1)
		pulsewidth = MIN_PULSE_WIDTH << 1;
	if (pulsewidth > MAX_PULSE_WIDTH << 1)
		pulsewidth = MAX_PULSE_WIDTH << 1;
	
	switch (channel) {
	case 1:
		OCR1A = pulsewidth;
		break;
	case 2:
		OCR1B = pulsewidth;
		break;
	default:
		return -1;
	}
	
	return 0;
}
