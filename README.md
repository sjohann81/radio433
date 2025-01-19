# Radio433

Radio433 is a library for AVR microcontrollers that can be used for
remote control and low speed communications over 433MHz or 315MHz bands.
Data rate is limited, and runs at 1000bps to 2000bps in a typical setup.
Radios used are 433MHz (ASK OOK) generic TX and RX modules (fs1000a
for TX, xy-mk-5v for RX).

## Antenna

C = f * lambda, where: C is the speed of light, f is the frequency and
lambda is the wavelength. The antenna should be chosen as 1/4th of the
wavelength for optimum resonance, so:

(299.792.458m/s) / 433.920.000Hz == 0.690893386m antenna should be
0.172723346m, ~17.3cm for 433MHz.

The antenna is ~17cm (1/4 wave length) straight wire. Other types of
antenna (such as helical or loaded coil) can be used. A straight 17.3cm
wire, perpendicular to the plane (also called a monopole antenna) works
fine.

## Frame format and data encoding

- A bit period (T) is dependent of the bit rate, and is 500us for
2000bps, for example. A frame is composed by a training strobe signal,
a frame sync word, a series of data words and a leadout word. Data
can be either raw or 4b5b encoded.
- Training strobe is a on/off signal *not* encoded nor inverted,
repeated twice, which translates to 20T (or 24T with 4b5b encoding).
- Frame sync is a ON pulse (5T bit time) followed by silence (5T bit
time) (or 6T + 6T).
- Payload is 10T (sync + 1 byte) (or 12T)
- Data is 1 .. 40 times 10T (max 40 bytes) (or 12T)
- Leadout is 10T (sync + 1 byte of zeroes) (or 12T)
- Frame format: 1 to 40 bytes, not including strobe (preamble), sync and
  leadout. Bytes are sent MSB first. each byte is encoded in a word and
  the word starts with a 1 and 0 pattern, in order to synchronize TX and
  RX clocks (something similar to UART start and stop bits).
- 40 bytes seems arbitrary, but packets must be small. 16 bytes may be
  too little, the next power of 2 (32) seems ok. We may need additional
  bytes for address, control and CRC bytes for data transport, so 40 
  bytes seems reasonable.
- In the worst case, a 40 byte payload is encoded in 540 bits at the
  physical layer (air).

STROBE (20T) - SYNC (10T) - PAYLOAD (10T) - DATA (10T for each word) - LEADOUT (10T) - raw encoding
STROBE (24T) - SYNC (12T) - PAYLOAD (12T) - DATA (12T for each word) - LEADOUT (12T) - 4b5b encoding

## Protocol design

- Data rate is low (1000bps) and depends on the interrupt frequency. Max
rate for these simple radios is around 5000bps. Tested with 100bps
to 5000bps baud rates.
- Implementation is interrupt driven. Interrupts happen only at the
baud rate frequency there is no need to oversample nonsense like 4x or
8x (such as in other libraries). We want to do other stuff with the
MCU (motor control, pwm, etc) while we wait for the radio.
- TX and RX sequencing for the radio happen inside the interrupt handler
in two finite state machines. Application TX and RX routines just
copy data to user buffers and signal start and stop conditions for
the FSMs.
- Oversampling is avoided because of two things: frame synchronization
and detection (with sync and leadout words) and individual word
synchronization. The receiver (RX) acts as a slave, and recalibrates
the timer following the master (TX) clock.
- This implementation includes only a way to transfer a data frame of
1 to 40 bytes. Payload headers, addressing, CRC and other things are
application defined.
- A training strobe is used to calibrate the receiver RF module AGC
(automatic gain control) for each frame. A sync word is used for
synchronization and frame detection. Two sync bits are used at the
beginning of each word for clock synchronization between TX and RX.
- Symetric preamble and frame coding ensures good DC balance for RF
transmission for large frames. Without DC balance, just a few bytes
would be sent before corruption. Using the UART directly with these
cheap RF modules is *bad* ideia.
- Variable frame size - may be adapted to a specific application. Larger
frames take more time to be transferred, so they are more likely to
suffer from data errors (byte synchronization, noise...).
- Addressing and CRC are implemented on top of raw frames by the
addition of a 4 byte header (destination and source address) and a 2
 byte footer. The API defines two sets of functions - one for direct
access to frames and another for packets with address, CRC and a
maximum of 32 bytes of user payload.

## 4b/5b coding

- Not perfectly DC balanced, but works well.
- Requires only a 16 entry encode table and a 32 entry decode table
- Tables presented here have much better balance than the original
(100Base-T Ethernet) values. The purpose here is different.
- 8 patterns with 2 ones (and 3 zeroes), other 8 with 3 ones (and 2
zeroes).
- Only a limited run of consecutive zeroes or ones can be found in the
data stream (maximum of 3 bits).
- Only two extra bits per byte.

|4b		|5b		|
| :------------ | :------------ |
| 0000		| 00101		|
| 0001		| 00110		|
| 0010		| 01001		|
| 0011		| 01010		|
| 0100		| 01011		|
| 0101		| 01100		|
| 0110		| 01101		|
| 0111		| 01110		|
| 1000		| 10001		|
| 1001		| 10010		|
| 1010		| 10011		|
| 1011		| 10100		|
| 1100		| 10101		|
| 1101		| 10110		|
| 1110		| 11001		|
| 1111		| 11010		|

## Radio modules considerations

RX:
These simple RX modules are very sensitive to the supply voltage, so
use a voltage regulator. Supply must be very close to 5v. I have
powered the RX (XY-MK-5v) from el cheapo 5v adapters (measured voltage
was 5.4v) and, although the sensibility of the receiver improved, one
module died in a matter of days. If you want to power the RX from USB,
measure the supply and ensure it's not more than 5v.

TX:
If you want more range, increase the TX module supply voltage. Most
simple TX modules can be powered with anything between 5v to 12v. 9v
to 12v is ideal. 12v may work best for longer range, but may fail if
TX and RX are too close. TX must be at least 1m far from RX. If placed
close, reduce supply voltage of the TX module.

Use line filtering in the modules such as a 100n + 100uF caps close to
the module.


## API

### Radio setup and direct frame TX and RX

- int radio433_setup(struct radio_data_s *radio, uint16_t baud, uint8_t direction);
- int radio433_tx(struct radio_data_s *radio, uint8_t *data, uint8_t payload);
- int radio433_rx(struct radio_data_s *radio, uint8_t *data, uint8_t *payload);

### Packet send and receive

- void radio433_addr(struct radio_data_s *radio, uint16_t address);
- int radio433_send(struct radio_data_s *radio, uint16_t dst_addr, uint8_t *data, uint8_t payload);
- int radio433_recv(struct radio_data_s *radio, uint16_t *src_addr, uint8_t *data, uint8_t *payload);
