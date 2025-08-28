#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>

#include "i2c.h"
#include "event.h"

// Tx DMA interrupt handler.
void
dma1_channel6_isr (void)
{
	DMA1_IFCR = DMA_IFCR_CTCIF6;
	DMA1_CCR6 = 0;

	if ((DMA1_CCR7 & DMA_CCR_EN) == 0) {
		event_raise(EVENT_I2C_FINISHED);
		return;
	}

	i2c_set_read_transfer_dir(I2C1);
	i2c_set_bytes_to_transfer(I2C1, DMA1_CNDTR7);
	i2c_send_start(I2C1);
	i2c_enable_autoend(I2C1);
}

// Rx DMA interrupt handler.
void
dma1_channel7_isr (void)
{
	DMA1_IFCR = DMA_IFCR_CTCIF7;
	DMA1_CCR7 = 0;
	event_raise(EVENT_I2C_FINISHED);
}

void
i2c_start_xfer (const uint8_t addr,
                const void *wr, const size_t wrlen,
                volatile void *rd, const size_t rdlen)
{
	// Disable the DMA channels. Note that some bits in the register will
	// remain set, so the value will not read back as zero.
	DMA1_CCR6 = 0;
	DMA1_CCR7 = 0;

	// Tx DMA
	DMA1_CMAR6  = (uint32_t) wr;
	DMA1_CPAR6  = (uint32_t) &I2C1_TXDR;
	DMA1_CNDTR6 = wrlen;
	DMA1_CCR6   = DMA_CCR_DIR
	            | DMA_CCR_MINC
	            | DMA_CCR_MSIZE_8BIT
	            | DMA_CCR_PSIZE_8BIT
		    | DMA_CCR_TCIE
	            | DMA_CCR_EN
	            ;

	// Rx DMA
	if (rdlen > 0) {
		DMA1_CMAR7  = (uint32_t) rd;
		DMA1_CPAR7  = (uint32_t) &I2C1_RXDR;
		DMA1_CNDTR7 = rdlen;
		DMA1_CCR7   = DMA_CCR_MINC
			    | DMA_CCR_MSIZE_8BIT
			    | DMA_CCR_PSIZE_8BIT
			    | DMA_CCR_TCIE
			    | DMA_CCR_EN
			    ;
	}

	i2c_peripheral_disable(I2C1);
	i2c_set_7bit_address(I2C1, addr);
	i2c_set_write_transfer_dir(I2C1);
	i2c_set_bytes_to_transfer(I2C1, wrlen);
	i2c_enable_txdma(I2C1);

	if (rdlen > 0) {
		i2c_disable_autoend(I2C1);
		i2c_enable_rxdma(I2C1);
	} else {
		i2c_enable_autoend(I2C1);
		i2c_disable_rxdma(I2C1);
	}

	i2c_peripheral_enable(I2C1);
	i2c_send_start(I2C1);
}

void
i2c_blocking_write (const uint8_t addr, const void *wr, const size_t wrlen)
{
	i2c_start_xfer(addr, wr, wrlen, NULL, 0);

	while (DMA1_CCR6 & DMA_CCR_EN)
		continue;

	while (I2C1_ISR & I2C_ISR_BUSY)
		continue;
}

void
i2c_init (void)
{
	// Setup PB6 (I2C1_SCL) as an output pin.
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO6);

	// Pulse the clock pin for 9 periods of 0.2us (based on High Speed 3.4
	// MHz mode) to reset the sensor if it is stuck.
	for (size_t i = 0; i < 18; i++) {
		(i & 1) == 0
			? gpio_set(GPIOB, GPIO6)
			: gpio_clear(GPIOB, GPIO6);

		__asm volatile (
			"   mov r0, #5 \n\t"
			"0: sub r0, #1 \n\t"
			"   cmp r0, #0 \n\t"
			"   bne 0b     \n\t"
			::: "r0", "cc"
		);
	}

	// Setup PB6 (I2C1_SCL) and PB7 (I2C1_SDA) in AF4.
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6 | GPIO7);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO6 | GPIO7);

	// Run off the system clock (72 MHz).
	rcc_set_i2c_clock_sysclk(I2C1);
	i2c_set_speed(I2C1, i2c_speed_fm_400k, 72);
	i2c_peripheral_disable(I2C1);

	// Enable the DMA Rx and DMA Tx interrupts.
	nvic_enable_irq(NVIC_DMA1_CHANNEL6_IRQ);
	nvic_enable_irq(NVIC_DMA1_CHANNEL7_IRQ);
}
