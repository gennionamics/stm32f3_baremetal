#include "clock.h"
#include "usart.h"

static void
init (void)
{
	clock_init();
	usart_init();
}

static void
loop (void)
{
	usart_printf("Hello world!\r\n");

	for (;;)
		__asm volatile("wfi");
}

int
main (void)
{
	init();
	loop();
}
