#pragma once

#include <stdint.h>

#include "acc_mag.h"
#include "gyro.h"

struct app_sensors {

	struct {
		uint32_t   ndata;
		struct acc data[2];
	} acc;

	struct {
		uint32_t    ndata;
		struct gyro data[2];
	} gyro;

	struct {
		uint32_t   ndata;
		struct mag data[2];
	} mag;

	struct {
		uint32_t ndata;
		int16_t  data[2];
	} temp;
};

extern void app_step (const struct app_sensors *s);
