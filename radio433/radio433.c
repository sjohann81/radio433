/* file:          radio433.c
 * description:   433MHz radio link library implementation
 * version:       v0.01
 * date:          01/2025
 * author:        Sergio Johann Filho <sergiojohannfilho@gmail.com>
 */
 
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <printf.h>
#include <crc.h>
#include <radio433.h>


#if ENCODE4B5B == 1
const uint8_t encode4b5b[] = {
	0x05, 0x06, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x19, 0x1a
};

const uint8_t decode4b5b[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
	0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x00,
	0x00, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x00,
	0x00, 0x0e, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif

volatile struct radio_data_s *radioptr;

ISR(TIMER2_COMPA_vect){
	uint8_t byte;
	static uint16_t rfdata;
	
	/* TX FSM */
	if (radioptr->direction == TX) {
		switch (radioptr->state) {
		case READY:
			ACT_PORT &= ~(1 << ACT_PIN);
			break;
		case START:
			/* received a START signal, then start TX */
			ACT_PORT |= (1 << ACT_PIN);
			radioptr->state = STROBE;
			radioptr->tbit = TSTROBE - 1;
			break;
		case STROBE:
			/* send a strobe signal to calibrate the RX AGC */
			TX_PORT ^= (1 << TX_PIN);
			if (radioptr->tbit > 0) {
				radioptr->tbit--;
			} else {
				radioptr->state = SYNC;
				radioptr->tbit = TSYNC - 1;
			}
			break;
		case SYNC:
			/* send sync pattern (half T high, half T low) */
			if (radioptr->tbit >= TSYNC >> 1) {
				TX_PORT |= (1 << TX_PIN);
			} else {
				TX_PORT &= ~(1 << TX_PIN);
			}
			if (radioptr->tbit > 0) {
				radioptr->tbit--;
			} else {
				radioptr->state = PAYLOAD;
				radioptr->tbit = TBYTE - 1;
			}
			break;
		case PAYLOAD:
			/* send the payload, one bit at a time. we start
			 * appending a 1 to 0 pattern to the front */
			byte = radioptr->payload;
#if ENCODE4B5B == 0
			rfdata = 0x200 | byte;
#else
			rfdata = 0x800 | (encode4b5b[byte >> 4] << 5) | encode4b5b[byte & 0xf];
#endif
			/* encode a zero or a one in the wire */
			if ((rfdata >> radioptr->tbit) & 1)
				TX_PORT |= (1 << TX_PIN);
			else
				TX_PORT &= ~(1 << TX_PIN);
			if (radioptr->tbit > 0) {
				radioptr->tbit--;
			} else {
				radioptr->state = DATA;
				radioptr->pcount = 0;
				radioptr->tbit = TBYTE - 1;
			}
			break;
		case DATA:
			/* send a data word, one bit at a time. we start
			 * appending a 1 to 0 pattern to the front */
			byte = radioptr->data[radioptr->pcount];
#if ENCODE4B5B == 0
			rfdata = 0x200 | byte;
#else
			rfdata = 0x800 | (encode4b5b[byte >> 4] << 5) | encode4b5b[byte & 0xf];
#endif
			/* encode a zero or a one in the wire */
			if ((rfdata >> radioptr->tbit) & 1)
				TX_PORT |= (1 << TX_PIN);
			else
				TX_PORT &= ~(1 << TX_PIN);
			if (radioptr->tbit > 0) {
				radioptr->tbit--;
			} else {
				/* more data to send? */
				if (radioptr->pcount < radioptr->payload) {
					radioptr->pcount++;
					radioptr->tbit = TBYTE - 1;
				} else {
					radioptr->state = LEADOUT;
					radioptr->tbit = TLEADOUT - 1;
				}
			}
			break;
		case LEADOUT:
			/* send leadout word (all zeroes) but append a
			 * 1 to 0 pattern to the front */
#if ENCODE4B5B == 0
			rfdata = 0x200;
#else
			rfdata = 0x800;
#endif
			if ((rfdata >> radioptr->tbit) & 1)
				TX_PORT |= (1 << TX_PIN);
			else
				TX_PORT &= ~(1 << TX_PIN);
			if (radioptr->tbit > 0) {
				radioptr->tbit--;
			} else {
				radioptr->state = READY;
			}
			break;
		default:
			break;
		};
	}
	
	/* RX FSM */
	if (radioptr->direction == RX) {
		switch (radioptr->state) {
		case START:
			radioptr->state = READY;
			radioptr->tbit = TSYNC - 1;
		case READY:
			ACT_PORT &= ~(1 << ACT_PIN);
			/* wait for a sync pattern to start RX */
			if (RX_PORT & (1 << RX_PIN)) {
				if (radioptr->tbit == (TSYNC >> 1))
					radioptr->state = SYNC;
				radioptr->tbit--;
			} else {
				radioptr->tbit = TSYNC - 1;
			}
			break;
		case SYNC:
			/* in sync */
			if (radioptr->tbit > 0) {
				radioptr->tbit--;
			} else {
				ACT_PORT |= (1 << ACT_PIN);
				radioptr->state = PAYLOAD;
				radioptr->tbit = TBYTE - 1;
			}
			rfdata = 0;
			break;
		case PAYLOAD:
			/* word sync bit, wait for the falling edge (1 to 0) and advance
			 * the timer by 1/8th period, so we interrupt a bit earlier
			 * and sample data at the right time for this word */
			if (radioptr->tbit == TBYTE - 1) {
				while (RX_PORT & (1 << RX_PIN));
				radioptr->tbit--;
				TCNT2 = OCR2A >> 3;
				break;
			}
		
			/* poll data - zero or one in the wire? */
			if (RX_PORT & (1 << RX_PIN))
				rfdata |= 1;
			
			/* still fetching data */	
			if (radioptr->tbit > 0) {
				rfdata <<= 1;
				radioptr->tbit--;
			} else {
			/* a word of data is ready, now decode it */
#if ENCODE4B5B == 0
				radioptr->payload = rfdata;
#else
				radioptr->payload = ((decode4b5b[rfdata >> 5] << 4) | decode4b5b[rfdata & 0x1f]);
#endif
				/* payload greater than expected or zero, not good */
				if (radioptr->payload == 0 || radioptr->payload > MAX_FRAME_SIZE) {
					radioptr->state = ERROR;
					radioptr->payload = 0;
					break;
				}
				rfdata = 0;
				radioptr->state = DATA;
				radioptr->pcount = 0;
				radioptr->tbit = TBYTE - 1;
			}
			break;
		case DATA:
			/* word sync bit, wait for the falling edge (1 to 0) and advance
			 * the timer by 1/8th period, so we interrupt a bit earlier
			 * and sample data at the right time for this word */
			if (radioptr->tbit == TBYTE - 1) {
				while (RX_PORT & (1 << RX_PIN));
				radioptr->tbit--;
				TCNT2 = OCR2A >> 3;
				break;
			}
			
			/* poll data - zero or one in the wire? */
			if (RX_PORT & (1 << RX_PIN))
				rfdata |= 1;
			
			/* still fetching data */
			if (radioptr->tbit > 0) {
				rfdata <<= 1;
				radioptr->tbit--;
			} else {
			/* a word of data is ready, now decode it */
#if ENCODE4B5B == 0
				radioptr->data[radioptr->pcount++] = rfdata;
#else
				radioptr->data[radioptr->pcount++] = ((decode4b5b[rfdata >> 5] << 4) | decode4b5b[rfdata & 0x1f]);
#endif
				rfdata = 0;
				/* any more data in the stream? */
				if (radioptr->pcount < radioptr->payload) {
					radioptr->state = DATA;
				} else {
					radioptr->state = LEADOUT;
				}
				radioptr->tbit = TBYTE - 1;
			}
			break;
		case LEADOUT:
			/* word sync */
			if (radioptr->tbit == TBYTE - 1) {
				while (RX_PORT & (1 << RX_PIN));
				radioptr->tbit--;
				TCNT2 = OCR2A >> 3;
				break;
			}
			
			/* wait for the leadout and move to the RECV state */
			if (radioptr->tbit == 0)
				radioptr->state = RECV;
			radioptr->tbit--;
			break;
		case RECV:
			break;
		case ERROR:
			break;
		default:
			break;
		};
	}
}


int radio433_setup(struct radio_data_s *radio, uint16_t baud, uint8_t direction)
{
	if (baud < 100 || baud > 5000)
		return ERR_CONFIG;
	
	/* setup TX, RX and ACT pins */
	TX_DIR |= (1 << TX_PIN);
	TX_PORT &= ~(1 << TX_PIN);
	
	RX_DIR &= ~(1 << RX_PIN);
	RX_PORT &= ~(1 << RX_PIN);

	ACT_DIR |= (1 << ACT_PIN);
	ACT_PORT &= ~(1 << ACT_PIN);
	
	cli();
	
	/* clear timer2 registers */
	TCNT2 = 0;
	TCCR2A = 0;
	TCCR2B = 0;

	/* turn on CTC mode, timer2
	 * clear on compare and match */
	TCCR2A |= (1 << WGM21);
	
	if (baud >= 1000) {
		OCR2A = ((F_CPU / 64) / baud) - 1;
		TCCR2B |= (1 << CS22);					/* clk / 64 (prescaler) */
	} else if (baud >= 250) {
		OCR2A = ((F_CPU / 256) / baud) - 1;
		TCCR2B |= (1 << CS22) | (1 << CS21);			/* clk / 256 (prescaler) */
	} else {
		OCR2A = ((F_CPU / 1024) / baud) - 1;
		TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);	/* clk / 1024 (prescaler) */
	}
	
	/* enable timer2 interrupts */
	TIMSK2 |= (1 << OCIE2A);

	/* initialize radio data structure */
	radio->payload = 0;
	radio->direction = direction;
	radio->state = READY;
	radio->tbit = TSYNC - 1;
	radio->address = 0;
	radioptr = radio;

	sei();
	
	return ERR_OK;
}

int radio433_tx(struct radio_data_s *radio, uint8_t *data, uint8_t payload)
{
	/* we are TX */
	if (radio->direction != TX)
		return ERR_CONFIG;
	
	/* transmission is happening, we should wait */
	if (radio->state != READY)
		return ERR_BUSY;
		
	/* payload cannot be larger than a frame */
	if (payload > MAX_FRAME_SIZE)
		payload = MAX_FRAME_SIZE;

	/* copy data from user buffer atomically and start the TX FSM */
	TIMSK2 &= ~(1 << OCIE2A);
	memcpy((char *)radio->data, data, payload);
	radio->payload = payload;
	radio->state = START;
	TCNT2 = 0;
	TIMSK2 |= (1 << OCIE2A);
	
	return ERR_OK;
}

int radio433_rx(struct radio_data_s *radio, uint8_t *data, uint8_t *payload)
{
	/* we are RX */
	if (radio->direction != RX)
		return ERR_CONFIG;
	
	/* reception failed or problem syncing */
	if (radio->state == ERROR) {
		radio->state = START;
		
		return ERR_FRAME_ERROR;
	}
	
	/* reception is happening or no data received */
	if (radio->state != RECV)
		return ERR_NO_DATA;
	
	/* copy data to user buffer atomically and restart the RX FSM */
	TIMSK2 &= ~(1 << OCIE2A);
	memcpy((char *)data, (char *)radio->data, radio->payload);
	*payload = radio->payload;
	radio->state = START;
	TIMSK2 |= (1 << OCIE2A);
	
	return ERR_OK;
}

void radio433_addr(struct radio_data_s *radio, uint16_t address)
{
	radio->address = address;
}

int radio433_send(struct radio_data_s *radio, uint16_t dst_addr, uint8_t *data, uint8_t payload)
{
	uint8_t buf[MAX_FRAME_SIZE];
	struct transport_s *const hdr = (struct transport_s *)buf;
	uint16_t *crc;
	int rval;
	
	if (!radio->address)
		return ERR_CONFIG;
	
	if (payload > MAX_DATA_SIZE)
		payload = MAX_DATA_SIZE;
	
	/* fill transport header and copy data */
	hdr->dst_addr = dst_addr;
	hdr->src_addr = radio->address;
	memcpy(buf + sizeof(struct transport_s), data, payload);
	
	/* calculate CRC and put it in place */
	crc = (uint16_t *)(buf + sizeof(struct transport_s) + payload);
	*crc = crc16ccitt(buf, sizeof(struct transport_s) + payload);
	
	/* send data frame (headers + payload + CRC) */
	rval = radio433_tx(radio, buf, sizeof(struct transport_s) + payload + 2);
	
	return rval;
}

int radio433_recv(struct radio_data_s *radio, uint16_t *src_addr, uint8_t *data, uint8_t *payload)
{
	uint8_t buf[MAX_FRAME_SIZE], size;
	struct transport_s *const hdr = (struct transport_s *)buf;
	uint16_t *crc;
	int rval;

	if (!radio->address)
		return ERR_CONFIG;

	do {
		/* try to receive a frame */
		rval = radio433_rx(radio, buf, &size);
		
		/* no data or something weird happened */
		if (rval != ERR_OK)
			return rval;
		
		crc = (uint16_t *)(buf + size - 2);
		
	/* check if this data is for us (this address or broadcast) */
	} while (hdr->dst_addr != radio->address && hdr->dst_addr != BCAST_ADDR);
	
	/* check CRC */
	if (crc16ccitt(buf, size - 2) != *crc)
		return ERR_CRC_ERROR;
		
	/* we are set, copy data */
	*src_addr = hdr->src_addr;
	*payload = size - sizeof(struct transport_s) - 2;
	memcpy(data, buf + sizeof(struct transport_s),
		size - sizeof(struct transport_s) - 2);
		
	return ERR_OK;
}
