#include <stdint.h>

#include "util.h"

// FIFO size at 800 Hz is 4 measurement triplets.
#define GYRO_FIFO_SIZE 4

// Rate measurement structure.
struct PACKED gyro_rate {
	int16_t x;
	int16_t y;
	int16_t z;
};

// Result buffer structure.
struct PACKED gyro {

	// Temperature data.
	uint8_t temp;

	// Status register.
	uint8_t status;

	// Angular rates.
	struct gyro_rate rates[GYRO_FIFO_SIZE];
};

extern const struct gyro *gyro_get_data (void);
extern void gyro_start_tx (void);
extern void gyro_config (void);
extern void gyro_reboot (void);
