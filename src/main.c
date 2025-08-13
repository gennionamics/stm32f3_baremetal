#include "app.h"
#include "clock.h"
#include "event.h"
#include "frame.h"
#include "gyro.h"
#include "led.h"
#include "usart.h"

static void
init (void)
{
	clock_init();
	frame_init();
	led_init();
	usart_init();

	// Reboot the sensors.
	gyro_reboot();

	// Give the sensors some time to reboot.
	for (unsigned int i = 0; i < 10; i++)
		if (event_test_and_clear(EVENT_FRAME) == false)
			__asm volatile("wfe");

	// Configure the sensors.
	gyro_config();
}

static void
loop (void)
{
	// Sensor data buffer.
	struct app_sensors buf = {};

	for (;;) {

		// Gyro data received: copy out the fresh data.
		if (event_test_and_clear(EVENT_GYRO_RX_FINISHED)) {

			// If two readings are available, move the oldest one
			// out of the way.
			if (buf.gyro.ndata == 2) {
				buf.gyro.data[0] = buf.gyro.data[1];
				buf.gyro.ndata--;
			}

			buf.gyro.data[buf.gyro.ndata++ & 1] = *gyro_get_data();
		}

		// Gyro data ready: start a read sequence.
		if (event_test_and_clear(EVENT_GYRO_DATA_READY))
			gyro_start_tx();

		// Step the application on every frame with sensor data.
		if (event_test_and_clear(EVENT_FRAME)) {

			// Step the application.
			app_step(&buf);

			// Reset the counters.
			buf.gyro.ndata = 0;
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
