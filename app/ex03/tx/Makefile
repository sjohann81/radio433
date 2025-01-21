MCU = atmega8				# atmega8/atmega32/atmega328p/atmega2560
CRYSTAL = 16000000
OPTIONS = ATMEGA8			# enable ATMEGA8/ATMEGA32 compatibility

SERIAL_DEV = /dev/ttyACM0
SERIAL_PROG = /dev/ttyACM0
SERIAL_BAUDRATE=57600

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

INC_DIRS  = -I ../../../lib -I ../../../radio433
CFLAGS = -g -mmcu=$(MCU) -Wall -Os -fno-inline-small-functions -fno-split-wide-types -D F_CPU=$(CRYSTAL) -D USART_BAUD=$(SERIAL_BAUDRATE) -D $(OPTIONS) --param=min-pagesize=0 $(INC_DIRS)

AVRDUDE_CONFIG=/usr/local/avr/gcc/etc/avrdude.conf
AVRDUDE_PART=m8				# m8/m32/m328p/m2560

#PROGRAMMER = bsd
PROGRAMMER = usbtiny
#PROGRAMMER = dasa -P $(SERIAL_PROG)
#PROGRAMMER = usbasp
#PROGRAMMER = arduino -P $(SERIAL_PROG)		# for uno
#PROGRAMMER = wiring -P $(SERIAL_PROG) -D	# for mega

all:
	$(CC) $(CFLAGS) -c ../../../lib/uart.c -o uart.o
	$(CC) $(CFLAGS) -c ../../../lib/printf.c -o printf.o
	$(CC) $(CFLAGS) -c ../../../lib/crc.c -o crc.o
	$(CC) $(CFLAGS) -c ../../../radio433/radio433.c -o radio433.o
	$(CC) $(CFLAGS) -c main.c -o main.o
	$(CC) $(CFLAGS) uart.o printf.o crc.o radio433.o main.o -o code.elf
	$(OBJCOPY) -R .eeprom -O ihex code.elf code.hex
	$(OBJDUMP) -d code.elf > code.lst
	$(OBJDUMP) -h code.elf > code.sec
	$(SIZE) code.elf

flash:
	avrdude -C $(AVRDUDE_CONFIG) -p $(AVRDUDE_PART) -U flash:w:code.hex -y -c $(PROGRAMMER)

debug: serial
	cat $(SERIAL_DEV)

# external high frequency crystal
fuses:
#	avrdude -C $(AVRDUDE_CONFIG) -p $(AVRDUDE_PART) -U lfuse:w:0xcf:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m -c $(PROGRAMMER)
	avrdude -C $(AVRDUDE_CONFIG) -p $(AVRDUDE_PART) -U lfuse:w:0xcf:m -U hfuse:w:0xd9:m -c $(PROGRAMMER)

# internal rc osc @ 1MHz, original factory config
fuses_osc:
	avrdude -C $(AVRDUDE_CONFIG) -p $(AVRDUDE_PART) -U lfuse:w:0x62:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m -c $(PROGRAMMER)

serial:
	stty ${SERIAL_BAUDRATE} raw cs8 -parenb -crtscts clocal cread ignpar ignbrk -ixon -ixoff -ixany -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke -F ${SERIAL_DEV}

serial_sim:
	socat -d -d  pty,link=/tmp/ttyS10,raw,echo=0 pty,link=/tmp/ttyS11,raw,echo=0

test:
	avrdude -C $(AVRDUDE_CONFIG) -p $(AVRDUDE_PART) -c $(PROGRAMMER)

parport:
	modprobe parport_pc

clean:
	rm -f *.o *.map *.elf *.sec *.lst *.hex *~
