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
	unsigned int i = 0, j = 0;

	for (;;)
	{
		// Print the frame count every 1/10 s.
		if (event_test_and_clear(EVENT_FRAME)) {
			if (++i == 20) {
				usart_printf("%d\r\n", j++);
				i = 0;
			}
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
