#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/timer.h>

#include "event.h"
#include "frame.h"

void
tim4_isr (void)
{
	TIM4_SR = 0;
	event_raise(EVENT_FRAME);
}

void
frame_init (void)
{
	// Setup TIM4 to serve 200 ticks per second.
	TIM4_CR1 = 0;
	TIM4_CNT = 0;
	TIM4_SR  = 0;

	// 72e6/200 = 360000 = 3600 * 100.
	TIM4_PSC = 3600 - 1;
	TIM4_ARR = 100 - 1;

	// Generate an update interrupt.
	TIM4_DIER = TIM_DIER_UIE;

	// Enable the interrupt.
	nvic_enable_irq(NVIC_TIM4_IRQ);

	// Start the timer.
	TIM4_CR1 = TIM_CR1_CEN;
}
