enum {STOP, FORWARD, REVERSE};
enum {PWM_25, PWM_50, PWM_100, PWM_250, PWM_500, PWM_1k, PWM_2k5, PWM_5k, PWM_10k, PWM_20k};

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
#define DC_CTRL2B		PB5
#define DC_PWMA			PB1
#define DC_PWMB			PB2

#define PWM_25_MAX		39999
#define PWM_50_MAX		19999
#define PWM_100_MAX		9999
#define PWM_250_MAX		3999
#define PWM_500_MAX		15999
#define PWM_1k_MAX		7999
#define PWM_2k5_MAX		3199
#define PWM_5k_MAX		1599
#define PWM_10k_MAX		799
#define PWM_20k_MAX		399
#endif

long map(long x, long in_min, long in_max, long out_min, long out_max);

void dc_init(uint8_t speed);
int dc_attach(uint8_t channel);
int dc_dettach(uint8_t channel);
int dc_direction(uint8_t channel, uint8_t direction);
int dc_write(uint8_t channel, uint16_t speed);
