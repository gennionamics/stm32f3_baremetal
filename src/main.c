#include "clock.h"

static void
init (void)
{
	clock_init();
}

static void
loop (void)
{
	for (;;)
		__asm volatile("wfi");
}

int
main (void)
{
	init();
	loop();
}
