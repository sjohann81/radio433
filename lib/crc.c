#include <stdint.h>

/*
CCITT CRC16
this version calculates the CRC16 on the fly, without the need of
a pre-computed table. we can keep the memory usage low in this uC!
*/

#define CRC_POLY			0x1021	

uint16_t crc16ccitt(uint8_t *data, uint16_t len)
{
	uint16_t crc = 0xffff, t, i;

  	while (len--) {
		t = *data++ << 8;
		crc = t ^ crc;
		for (i = 0; i < 8; i++) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ CRC_POLY;
			else
				crc <<= 1;
		}
	}

	return crc;
}
