#define SERVO_PORT		PORTD
#define SERVO_DIR		DDRD
#define MIN_PULSE_WIDTH		400
#define MAX_PULSE_WIDTH		2600
#define DEFAULT_PULSE_WIDTH	1500
#define SYNC_PERIOD		20000
#define MAX_CHANNELS		8

struct servo_t {
	uint8_t pin;
	uint8_t count;
	uint8_t countdown;
	uint8_t enabled;
};

void servo0_init();
int servo0_attach(uint8_t channel, uint8_t pin);
int servo0_dettach(uint8_t channel);
int servo0_write(uint8_t channel, uint16_t pulsewidth);
void servo1_init();
int servo1_attach(uint8_t channel);
int servo1_dettach(uint8_t channel);
int servo1_write(uint8_t channel, uint16_t pulsewidth);
