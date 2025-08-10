#include <stdint.h>

#include "app.h"
#include "frame.h"
#include "led.h"
#include "usart.h"

static uint32_t tick, sec;

void
app_step (void)
{
	// Animate the LEDs: one circle per 200 frames (1 s).
	led_set_angle(tick * 256 / FRAME_RATE);

	// Print a second counter.
	if (++tick == FRAME_RATE) {
		usart_printf("%03lu\r\n", sec++);
		tick = 0;
	}
}
