#include <libopencm3/cm3/common.h>

#include <stdint.h>

#include "event.h"

static volatile uint32_t flags;

void
event_raise (const enum Event event)
{
	BBIO_SRAM(&flags, event) = 1;
	__asm volatile("sev");
}

bool
event_test_and_clear (const enum Event event)
{
	const bool isset = BBIO_SRAM(&flags, event);

	if (isset)
		BBIO_SRAM(&flags, event) = 0;

	return isset;
}
