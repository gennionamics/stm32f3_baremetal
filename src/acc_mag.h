#include <stdint.h>

#include "util.h"

// FIFO size at 1344 Hz is 7 measurement triplets.
#define ACC_FIFO_SIZE 7

// Accelerometer measurement structure.
struct PACKED acc_mag_data {
	int16_t x;
	int16_t y;
	int16_t z;
};

// Accelerometer data structure.
struct PACKED acc {
	struct acc_mag_data acc[ACC_FIFO_SIZE];
};

// Magnetometer data structure.
struct PACKED mag {
	struct acc_mag_data mag;
};

extern const struct acc *acc_mag_get_acc  (void);
extern const struct mag *acc_mag_get_mag  (void);
extern const int16_t    *acc_mag_get_temp (void);

extern void acc_mag_step   (void);
extern void acc_mag_config (void);
extern void acc_mag_reboot (void);
