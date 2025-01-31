enum {STOP, FORWARD, REVERSE};

#ifdef ALT_DC_CONFIG

#define DC_PORT			PORTD
#define DC_DIR			DDRD
#define DC_CTRL1A		PD1
#define DC_CTRL2A		PD2
#define DC_CTRL1B		PD3
#define DC_CTRL2B		PD4
#define DC_PWMA			PD6
#define DC_PWMB			PD5

#else

#define DC_PORT			PORTB
#define DC_DIR			DDRB
#define DC_CTRL1A		PB3
#define DC_CTRL2A		PB4
#define DC_CTRL1B		PB0
#define DC_CTRL2B		PB3
#define DC_PWMA			PB1
#define DC_PWMB			PB2

#endif

long map(long x, long in_min, long in_max, long out_min, long out_max);

void dc_init();
int dc_attach(uint8_t channel);
int dc_dettach(uint8_t channel);
int dc_direction(uint8_t channel, uint8_t direction);
int dc_write(uint8_t channel, uint16_t speed);
