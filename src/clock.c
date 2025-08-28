#include <libopencm3/stm32/rcc.h>

#include "clock.h"
#include "util.h"

static const uint32_t map_clock[] = {
	RCC_DMA1,
	RCC_DMA2,
	RCC_GPIOA,
	RCC_GPIOB,
	RCC_GPIOC,
	RCC_GPIOE,
	RCC_I2C1,
	RCC_SPI1,
	RCC_SYSCFG,
	RCC_TIM3,
	RCC_TIM4,
	RCC_TIM8,
	RCC_USART1,
};

static const uint32_t map_reset[] = {
	RST_GPIOA,
	RST_GPIOB,
	RST_GPIOC,
	RST_GPIOE,
	RST_I2C1,
	RST_SPI1,
	RST_SYSCFG,
	RST_TIM3,
	RST_TIM4,
	RST_TIM8,
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
