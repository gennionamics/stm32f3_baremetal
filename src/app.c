#include <stdint.h>

#include "app.h"
#include "frame.h"
#include "led.h"
#include "usart.h"

static uint32_t tick, sec, ngyro;

void
app_step (const struct app_sensors *s)
{
	// Animate the LEDs: one circle per 200 frames (1 s).
	led_set_angle(tick * 256 / FRAME_RATE);

	// Print a second counter.
	if (++tick == FRAME_RATE) {
		usart_printf("%03lu gyro(%lu)\r\n", sec++, ngyro);
		tick  = 0;
		ngyro = 0;
	}

	// Accumulate the number of gyro readings.
	ngyro += s->gyro.ndata * GYRO_FIFO_SIZE;
}
