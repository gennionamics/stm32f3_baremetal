#pragma once

#include <stdbool.h>

enum Event {
	EVENT_ACC_DATA_READY,
	EVENT_ACC_FINISHED,
	EVENT_ACC_TEMP_FINISHED,
	EVENT_FRAME,
	EVENT_GYRO_DATA_READY,
	EVENT_GYRO_RX_FINISHED,
	EVENT_I2C_FINISHED,
	EVENT_MAG_DATA_READY,
	EVENT_MAG_FINISHED,

	EVENT_count
};

extern void event_raise (const enum Event);
extern bool event_test_and_clear (const enum Event event);
