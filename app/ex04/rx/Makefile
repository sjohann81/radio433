# atmega8/atmega32/atmega328p/atmega2560
MCU = atmega328p
CRYSTAL = 16000000
# enable ATMEGA8/ATMEGA32 compatibility
OPTIONS = NO #ATMEGA8

SERIAL_DEV = /dev/ttyUSB0
# pro mini requires an external adapter, may use /dev/ttyUSB0
SERIAL_PROG = /dev/ttyUSB0
SERIAL_BAUDRATE=57600
# 57600 for arduino pro mini, 115200 for others
SERIAL_PROG_BAUDRATE=57600

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

INC_DIRS  = -I ../../../lib -I ../../../motor -I ../../../radio433
CFLAGS = -g -mmcu=$(MCU) -Wall -Os -fno-inline-small-functions -fno-split-wide-types -D F_CPU=$(CRYSTAL) -D USART_BAUD=$(SERIAL_BAUDRATE) -D $(OPTIONS) $(INC_DIRS)

#PROGRAMMER = bsd
#PROGRAMMER = usbtiny
#PROGRAMMER = dasa -P $(SERIAL_PROG)
#PROGRAMMER = usbasp
# for arduino uno, pro mini
PROGRAMMER = arduino -P $(SERIAL_PROG)
# for arduino mega
#PROGRAMMER = wiring -P $(SERIAL_PROG) -D

all:
	$(CC) $(CFLAGS) -c ../../../lib/uart.c -o uart.o
	$(CC) $(CFLAGS) -c ../../../lib/printf.c -o printf.o
	$(CC) $(CFLAGS) -c ../../../lib/crc.c -o crc.o
	$(CC) $(CFLAGS) -c ../../../lib/adc.c -o adc.o
	$(CC) $(CFLAGS) -c ../../../motor/dc.c -o dc.o
	$(CC) $(CFLAGS) -c ../../../motor/servo.c -o servo.o
	$(CC) $(CFLAGS) -c ../../../radio433/radio433.c -o radio433.o
	$(CC) $(CFLAGS) -c main.c -o main.o
	$(CC) $(CFLAGS) uart.o printf.o crc.o adc.o dc.o servo.o \
		radio433.o main.o -o code.elf
	$(OBJCOPY) -R .eeprom -O ihex code.elf code.hex
	$(OBJDUMP) -d code.elf > code.lst
	$(OBJDUMP) -h code.elf > code.sec
	$(SIZE) code.elf

flash:
	avrdude -p $(MCU) -c $(PROGRAMMER) -b $(SERIAL_PROG_BAUDRATE) -U flash:w:code.hex

debug: serial
	cat $(SERIAL_DEV)

# external high frequency crystal
fuses:
	avrdude -p $(MCU) -U lfuse:w:0xcf:m -U hfuse:w:0xd9:m -c $(PROGRAMMER)

# internal rc osc @ 1MHz, original factory config
fuses_osc:
	avrdude -p $(MCU) -U lfuse:w:0x62:m -U hfuse:w:0xd9:m -c $(PROGRAMMER)

serial:
	stty ${SERIAL_BAUDRATE} raw cs8 -parenb -crtscts clocal cread ignpar ignbrk -ixon -ixoff -ixany -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -F ${SERIAL_DEV}

serial_sim:
	socat -d -d  pty,link=/tmp/ttyS10,raw,echo=0 pty,link=/tmp/ttyS11,raw,echo=0

test:
	avrdude -p $(MCU) -c $(PROGRAMMER) -b $(SERIAL_PROG_BAUDRATE)
	
parport:
	modprobe parport_pc

clean:
	rm -f *.o *.map *.elf *.sec *.lst *.hex *~
