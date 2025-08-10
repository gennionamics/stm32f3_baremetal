#include <libopencm3/stm32/rcc.h>

#include "clock.h"
#include "util.h"

static const uint32_t map_clock[] = {
	RCC_DMA1,
	RCC_GPIOC,
	RCC_USART1,
};

static const uint32_t map_reset[] = {
	RST_GPIOC,
	RST_USART1,
};

void
clock_init (void)
{
	// Run at 72 MHz from the 8 MHz external clock input.
	rcc_clock_setup_pll(&rcc_hse8mhz_configs[RCC_CLOCK_HSE8_72MHZ]);

	// Enable the peripheral clocks.
	FOREACH (map_clock, clock)
		rcc_periph_clock_enable(*clock);

	// Reset the peripherals.
	FOREACH (map_reset, reset)
		rcc_periph_reset_pulse(*reset);
}
