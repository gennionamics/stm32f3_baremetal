#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include "util.h"

#define LD3	GPIO9
#define LD4	GPIO8
#define LD5	GPIO10
#define LD6	GPIO15
#define LD7	GPIO11
#define LD8	GPIO14
#define LD9	GPIO12
#define LD10	GPIO13

#define LD_ALL	(LD3 | LD4 | LD5 | LD6 | LD7 | LD8 | LD9 | LD10)

// LED level array. This contains the value to assign to GPIOE_ODR for each
// brightness level at each timer tap.
static uint16_t led_levels[16];

// Array of current LED brightnesses.
static uint8_t cur_levels[8];

// Map the LEDs. The one at 12 o'clock is #0.
static const uint16_t map_led[] = {
	LD3, LD5, LD7, LD9, LD10, LD8, LD6, LD4,
};

// Taps array. The points in the PWM pulse at which the GPIOs are updated. LEDs
// do not have a linear brightness scale, so it is necessary to manually map
// the brightness levels to PWM tap points.
// Important: the tap values are duplicated because the same tap values need to
// be written to CCR3 and CCR4 at the same time. The last tap value must also
// return back to zero to close the loop.
static const uint16_t taps[32] = {
	 1,  1,  2,  2,  3,  3,  4,  4,
	 5,  5,  6,  6,  7,  7,  9,  9,
	11, 11, 13, 13, 15, 15, 18, 18,
	21, 21, 24, 24, 29, 29,  0,  0,
};

void
led_set (const uint8_t led, const uint8_t level)
{
	// Do nothing if the value is unchanged.
	if (level == cur_levels[led])
		return;

	// If the level increased, set the extra bits from the bottom upward.
	if (level > cur_levels[led]) {
		for (int i = cur_levels[led]; i < level; i++) {
			led_levels[i] |= map_led[led];
		}
	}

	// If the level decreased, clear the extra bits from the top down.
	else {
		for (int i = cur_levels[led] - 1; i >= level; i--) {
			led_levels[i] &= ~map_led[led];
		}
	}

	// Remember the current level.
	cur_levels[led] = level;
}

void
led_set_all (const uint8_t *levels)
{
	for (uint32_t led = 0; led < NELEM(map_led); led++)
		led_set(led, levels[led]);
}

static uint8_t
brad_dist (const uint8_t pole, const uint8_t brad)
{
	const int8_t d = brad - pole;

	if (d < -31 || d > 31)
		return 0;

	return 15 - (d < 0 ? -d : d) / 2;
}

void
led_set_angle (const uint8_t brad)
{
	uint8_t values[8];

	for (uint32_t i = 0; i < NELEM(values); i++)
		values[i] = brad_dist(i * 32, brad);

	led_set_all(values);
}

void
led_init (void)
{
	// Configure the LED GPIOs as outputs.
	gpio_mode_setup(
		GPIOE,
		GPIO_MODE_OUTPUT,
		GPIO_PUPD_NONE,
		LD_ALL
	);

	// Configure the output options.
	gpio_set_output_options(
		GPIOE,
		GPIO_OTYPE_PP,
		GPIO_OSPEED_50MHZ,
		LD_ALL
	);

	// Configure DMA2 Channel 1 to move values from the LED levels array to
	// the GPIOE output register in circular mode. This DMA request will
	// serve one transfer for every TIM8_CH3 event.
	DMA2_CCR1   = 0;
	DMA2_CMAR1  = (uint32_t) led_levels;
	DMA2_CPAR1  = (uint32_t) &GPIOE_ODR;
	DMA2_CNDTR1 = NELEM(led_levels);
	DMA2_CCR1   =  DMA_CCR_CIRC
	            | DMA_CCR_DIR
	            | DMA_CCR_MINC
	            | DMA_CCR_MSIZE_16BIT
	            | DMA_CCR_PSIZE_32BIT
	            | DMA_CCR_EN
	            ;

	// Configure DMA2 Channel 5 to move values from the taps array to
	// TIM8's CCR2 and CCR3 registers in circular mode. This DMA request
	// will serve one transfer for every TIM8_CH2 event.
	DMA2_CCR5   = 0;
	DMA2_CMAR5  = (uint32_t) taps;
	DMA2_CPAR5  = (uint32_t) &TIM8_DMAR;
	DMA2_CNDTR5 = NELEM(taps);
	DMA2_CCR5   = DMA_CCR_CIRC
	            | DMA_CCR_DIR
	            | DMA_CCR_MINC
	            | DMA_CCR_MSIZE_16BIT
	            | DMA_CCR_PSIZE_32BIT
	            | DMA_CCR_EN
	            ;

	// Setup TIM8 to serve DMA requests for CCR3 and CCR4.
	TIM8_CR1 = 0;
	TIM8_CNT = 0;
	TIM8_SR  = 0;

	// One tick is 1 microsecond.
	TIM8_PSC = 72 - 1;

	// PWM width: 200 ticks.
	TIM8_ARR = 200 - 1;

	// Configure CCR2 and CCR3 to issue DMA requests immediately after
	// starting.
	TIM8_CCR2 = 0;
	TIM8_CCR3 = 0;
	TIM8_DIER = TIM_DIER_CC2DE | TIM_DIER_CC3DE;
	TIM8_CCER = TIM_CCER_CC2E | TIM_CCER_CC3E;

	// Perform two transfers starting at the word offset of CCR2.
	TIM8_DCR = 0x010E;

	// Start the timer.
	TIM8_CR1 = TIM_CR1_CEN;
}
