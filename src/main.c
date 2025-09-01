#include "app.h"
#include "clock.h"
#include "event.h"
#include "frame.h"
#include "usart.h"

static void
init (void)
{
	clock_init();
	frame_init();
	usart_init();
}

static void
loop (void)
{
	for (;;) {

		// Step the application on every frame.
		if (event_test_and_clear(EVENT_FRAME))
			app_step();

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
