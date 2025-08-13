#pragma once

#include <stdint.h>

#include "gyro.h"

struct app_sensors {
	struct {
		uint32_t ndata;
		struct gyro data[2];
	} gyro;
};

extern void app_step (const struct app_sensors *s);
