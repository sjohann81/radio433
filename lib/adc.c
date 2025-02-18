/* file:          adc.c
 * description:   ADC control / analog input driver
 * version:       v0.01
 * date:          01/2025
 * author:        Sergio Johann Filho <sergiojohannfilho@gmail.com>
 */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void adc_init()
{
	/* enable ADC */
	ADCSRA = (1 << ADEN);
	/* AVcc with external capacitor at AREF pin */
	ADMUX |= (1 << REFS0);
	/* select 128 prescaler, enable ADC */
	ADCSRA |= (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
}

void adc_set_channel(uint8_t ch)
{
	/* ADC channel select */
	ADMUX &= ~0xf;
	ADMUX |= ch & 0xf;
	_delay_us(300);
}

uint16_t adc_read()
{
	/* start a conversion */
	ADCSRA |= (1 << ADSC);
	/* wait for the conversion */
	while (!(ADCSRA & (1 << ADSC)));
	/* reset ADC, conversion complete */
	ADCSRA |= (1 << ADIF);
	_delay_us(300);

	return ADC;
}

