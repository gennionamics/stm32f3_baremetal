#include <stdint.h>

#include "app.h"
#include "frame.h"
#include "usart.h"

static uint32_t tick, sec;

void
app_step (void)
{
	// Print a second counter.
	if (++tick == FRAME_RATE) {
		usart_printf("%03lu\r\n", sec++);
		tick = 0;
	}
}
