#define TX_PORT			PORTC
#define TX_DIR			DDRC
#define TX_PIN			PC2
#define RX_PORT			PINC
#define RX_DIR			DDRC
#define RX_PIN			PC3

#define ENCODE4B5B		1
#define MAX_FRAME_SIZE		40			// 32 bytes for user data + 8 bytes for length, address, options, CRC...
#define MAX_DATA_SIZE		32			// 32 bytes for user data

#if ENCODE4B5B == 0
#define TSTROBE			20			// training preamble strobe length
#define TSYNC			10			// half period high, half low
#define TBYTE			10			// period for 1 byte using raw coding
#define TLEADOUT		10			// period of silence
#else
#define TSTROBE			24			// training preamble strobe length
#define TSYNC			12			// half period high, half low
#define TBYTE			12			// period for 1 byte using 4b5b coding
#define TLEADOUT		12			// period of silence
#endif

#define ERR_OK			0
#define ERR_NO_DATA		-1
#define ERR_BUSY		-2
#define ERR_FRAME_ERROR		-3
#define ERR_CRC_ERROR		-4
#define ERR_CONFIG		-5

enum radio_state {
	READY, START, STROBE, SYNC, PAYLOAD, DATA, LEADOUT, RECV, ERROR
};

enum radio_dir {
	NONE, TX, RX
};

struct radio_data_s {
	volatile uint8_t data[MAX_FRAME_SIZE];
	volatile uint8_t payload;
	volatile uint8_t pcount;
	volatile uint8_t tbit;
	volatile uint8_t state;
	volatile uint8_t direction;
	uint16_t address;
};

int radio433_setup(struct radio_data_s *radio, uint16_t baud, uint8_t direction);
int radio433_tx(struct radio_data_s *radio, uint8_t *data, uint8_t payload);
int radio433_rx(struct radio_data_s *radio, uint8_t *data, uint8_t *payload);

#define BCAST_ADDR		0xffff

struct transport_s {
	uint16_t dst_addr;
	uint16_t src_addr;
};

void radio433_addr(struct radio_data_s *radio, uint16_t address);
int radio433_send(struct radio_data_s *radio, uint16_t dst_addr, uint8_t *data, uint8_t payload);
int radio433_recv(struct radio_data_s *radio, uint16_t *src_addr, uint8_t *data, uint8_t *payload);
