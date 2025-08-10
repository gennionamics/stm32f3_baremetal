#include <stdarg.h>
#include <stdio.h>

#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

static char txbuf[128];

void
usart_printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	const int n = vsnprintf(txbuf, sizeof(txbuf), fmt, ap);
	va_end(ap);

	if (n <= 0)
		return;

	DMA1_CCR4   = 0;
	DMA1_CNDTR4 = (uint16_t) n;
	DMA1_CCR4   = DMA_CCR_DIR
		    | DMA_CCR_MINC
		    | DMA_CCR_MSIZE_8BIT
		    | DMA_CCR_PSIZE_8BIT
		    | DMA_CCR_EN
		    ;
}

void
usart_init (void)
{
	// Setup PC4 as the USART1_TX pin in Alternate Function 7.
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO4);
	gpio_set_af(GPIOC, GPIO_AF7, GPIO4);

	// Run the USART at 921600 baud.
	usart_set_baudrate(USART1, 921600);

	// Enable the USART in 8N1 Tx-only mode.
	USART1_CR3 = USART_CR3_DMAT;
	USART1_CR2 = 0;
	USART1_CR1 = USART_CR1_TE | USART_CR1_UE;

	// Set up the DMA src/dst registers.
	DMA1_CMAR4 = (uint32_t) txbuf;
	DMA1_CPAR4 = (uint32_t) &USART1_TDR;
}
