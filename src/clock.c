#include <libopencm3/stm32/rcc.h>

#include "clock.h"

void
clock_init (void)
{
	// Run at 72 MHz from the 8 MHz external clock input.
	rcc_clock_setup_pll(&rcc_hse8mhz_configs[RCC_CLOCK_HSE8_72MHZ]);
}
