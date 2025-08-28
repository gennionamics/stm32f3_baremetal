#include <stdint.h>

#include "app.h"
#include "frame.h"
#include "led.h"
#include "usart.h"

static uint32_t tick, sec, nacc, ngyro, nmag, ntemp;

void
app_step (const struct app_sensors *s)
{
	// Animate the LEDs: one circle per 200 frames (1 s).
	led_set_angle(tick * 256 / FRAME_RATE);

	// Print a second counter.
	if (++tick == FRAME_RATE) {
		usart_printf("%03lu acc(%lu) gyro(%lu) mag(%lu) temp(%lu)\r\n",
		             sec++,
		             nacc, ngyro, nmag, ntemp);
		tick  = 0;
		nacc  = 0;
		ngyro = 0;
		nmag  = 0;
		ntemp = 0;
	}

	// Accumulate the number of gyro readings.
	nacc  += s->acc.ndata  * ACC_FIFO_SIZE;
	ngyro += s->gyro.ndata * GYRO_FIFO_SIZE;
	nmag  += s->mag.ndata;
	ntemp += s->temp.ndata;
}
