#include <string.h>

#include "acc_mag.h"
#include "app.h"
#include "clock.h"
#include "event.h"
#include "frame.h"
#include "gyro.h"
#include "i2c.h"
#include "led.h"
#include "usart.h"

static void
init (void)
{
	clock_init();
	frame_init();
	i2c_init();
	led_init();
	usart_init();

	// Reboot the sensors.
	acc_mag_reboot();
	gyro_reboot();

	// Give the sensors some time to reboot.
	for (unsigned int i = 0; i < 10; i++)
		if (event_test_and_clear(EVENT_FRAME) == false)
			__asm volatile("wfe");

	// Configure the sensors.
	acc_mag_config();
	gyro_config();
}

static inline void
store_data (const void *data, uint32_t *ndata, void *dst, const size_t size)
{
	// If two readings are available, discard the oldest one.
	if (*ndata == 2) {
		memcpy(dst, dst + size, size);
		*ndata = 1;
	}

	// Copy the data to the slot.
	memcpy(dst + (*ndata)++ * size, data, size);
}

static void
loop (void)
{
	// Sensor data buffer.
	struct app_sensors buf = {};

	for (;;) {

		// Gyro data received: copy out the fresh data.
		if (event_test_and_clear(EVENT_GYRO_RX_FINISHED))
			store_data(gyro_get_data(), &buf.gyro.ndata,
			           buf.gyro.data, sizeof (*buf.gyro.data));

		// Gyro data ready: start a read sequence.
		if (event_test_and_clear(EVENT_GYRO_DATA_READY))
			gyro_start_tx();

		// Step the accelerometer/magnetometer state machine.
		acc_mag_step();

		// Accelerometer data received: copy out the fresh data.
		if (event_test_and_clear(EVENT_ACC_FINISHED))
			store_data(acc_mag_get_acc(), &buf.acc.ndata,
			           buf.acc.data, sizeof (*buf.acc.data));

		// Temperature data received: copy out the fresh data.
		if (event_test_and_clear(EVENT_ACC_TEMP_FINISHED))
			store_data(acc_mag_get_temp(), &buf.temp.ndata,
			           buf.temp.data, sizeof (*buf.temp.data));

		// Magnetometer data received: copy out the fresh data.
		if (event_test_and_clear(EVENT_MAG_FINISHED))
			store_data(acc_mag_get_mag(), &buf.mag.ndata,
			           buf.mag.data, sizeof (*buf.mag.data));

		// Step the application on every frame with sensor data.
		if (event_test_and_clear(EVENT_FRAME)) {

			// Step the application.
			app_step(&buf);

			// Reset the counters.
			buf.acc.ndata  = 0;
			buf.gyro.ndata = 0;
			buf.mag.ndata  = 0;
			buf.temp.ndata = 0;
		}

		// Sleep until the next event occurs.
		__asm volatile("wfe");
	}
}

int
main (void)
{
	init();
	loop();
}
