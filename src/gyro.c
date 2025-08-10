#include <stddef.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>

#include "event.h"
#include "gyro.h"

#define SPI_HEADER_RD			0b10000000
#define SPI_HEADER_WR			0b00000000
#define SPI_HEADER_MS			0b01000000

#define CTRL_REG1_DR_800_BW_110		0b11110000
#define CTRL_REG1_PD_NORMAL		0b00001000
#define CTRL_REG1_ZEN			0b00000100
#define CTRL_REG1_YEN			0b00000010
#define CTRL_REG1_XEN			0b00000001

#define CTRL_REG2_HPM_NORMAL		0b00000000
#define CTRL_REG2_HPCF_56		0b00000000

#define CTRL_REG3_I1_INT1		0b10000000
#define CTRL_REG3_I1_BOOT		0b01000000
#define CTRL_REG3_H_LACTIVE		0b00100000
#define CTRL_REG3_PP_OD			0b00010000
#define CTRL_REG3_I2_DRDY		0b00001000
#define CTRL_REG3_I2_WTM		0b00000100
#define CTRL_REG3_I2_ORUN		0b00000010
#define CTRL_REG3_I2_EMPTY		0b00000001

#define CTRL_REG4_BLE_BIG		0b01000000
#define CTRL_REG4_FS_245		0b00000000
#define CTRL_REG4_FS_500		0b00010000
#define CTRL_REG4_FS_1000		0b00100000
#define CTRL_REG4_FS_2000		0b00110000
#define CTRL_REG4_SIM_3WIRE		0b00000001

#define CTRL_REG5_BOOT_REBOOT		0b10000000
#define CTRL_REG5_FIFO_EN		0b01000000
#define CTRL_REG5_HPEN			0b00010000

#define FIFO_CTRL_REG_FM_BYPASS		0b00000000
#define FIFO_CTRL_REG_FM_FIFO		0b00100000
#define FIFO_CTRL_REG_FM_STREAM		0b01000000

// Register address enumeration.
enum {
	REG_WHO_AM_I  = 0x0F,
	REG_CTRL_REG1 = 0x20,
	REG_CTRL_REG2,
	REG_CTRL_REG3,
	REG_CTRL_REG4,
	REG_CTRL_REG5,
	REG_REFERENCE,
	REG_OUT_TEMP,
	REG_STATUS_REG,
	REG_OUT_X_L,
	REG_OUT_X_H,
	REG_OUT_Y_L,
	REG_OUT_Y_H,
	REG_OUT_Z_L,
	REG_OUT_Z_H,
	REG_FIFO_CTRL_REG,
};

// Reception buffer structure.
static struct PACKED {

	// The first byte to be exchanged is line noise.
	uint8_t dummy;

	// Reception data structure.
	struct gyro gyro;

} rxbuf;

// Reboot sequence.
static const uint8_t config_reboot[] = {
	SPI_HEADER_WR | SPI_HEADER_MS | REG_CTRL_REG5,
	CTRL_REG5_BOOT_REBOOT,
};

// Control register configuration sequence.
static const uint8_t config_ctrl[] = {
	SPI_HEADER_WR | SPI_HEADER_MS | REG_CTRL_REG1,
	CTRL_REG1_DR_800_BW_110 | CTRL_REG1_PD_NORMAL | CTRL_REG1_ZEN | CTRL_REG1_YEN | CTRL_REG1_XEN,
	CTRL_REG2_HPM_NORMAL | CTRL_REG2_HPCF_56,
	CTRL_REG3_I2_WTM,
	CTRL_REG4_FS_2000,
	CTRL_REG5_FIFO_EN,
};

// FIFO configuration sequence.
static const uint8_t config_fifo[] = {
	SPI_HEADER_WR | SPI_HEADER_MS | REG_FIFO_CTRL_REG,
	FIFO_CTRL_REG_FM_STREAM | GYRO_FIFO_SIZE,
};

// Control sequence to read from the FIFO up to the watermark.
static const uint8_t read_fifo[sizeof (rxbuf)] = {
	SPI_HEADER_RD | SPI_HEADER_MS | REG_OUT_TEMP,
};

const struct gyro *
gyro_get_data (void)
{
	return &rxbuf.gyro;
}

static inline void
spi_select (void)
{
	TIM3_CCMR1 = TIM_CCMR1_OC2M_INACTIVE;
}

static inline void
spi_deselect (void)
{
	TIM3_CCMR1 = TIM_CCMR1_OC2M_ACTIVE;
}

void
exti1_isr (void)
{
	exti_reset_request(EXTI1);
	event_raise(EVENT_GYRO_DATA_READY);
}

void
dma1_channel2_isr (void)
{
	spi_deselect();
	DMA1_IFCR = DMA_IFCR_CTCIF2;
	event_raise(EVENT_GYRO_RX_FINISHED);

	// If PE1 is still asserted, the FIFO is still above its watermark.
	// This can happen at bootup when the FIFO is full. Raise an event so
	// that the main loop can immediately start another read (after
	// stashing the current set of measurements).
	if (gpio_get(GPIOE, GPIO1) != 0)
		event_raise(EVENT_GYRO_DATA_READY);
}

static void
start_transfer (const void *txbuf, const uint32_t len)
{
	DMA1_CCR3 = 0;
	DMA1_CCR2 = 0;

	// The reference manual mentions that the DMA channels have to be
	// enabled in the right order. Rx DMA needs to be set up first.
	SPI1_CR2 = SPI_CR2_RXDMAEN;

	// Configure and enable Rx DMA.
	DMA1_CMAR2  = (uint32_t) &rxbuf;
	DMA1_CPAR2  = (uint32_t) &SPI1_DR;
	DMA1_CNDTR2 = len;
	DMA1_CCR2   = DMA_CCR_MINC
	            | DMA_CCR_MSIZE_8BIT
	            | DMA_CCR_PSIZE_8BIT
		    | DMA_CCR_TCIE
	            | DMA_CCR_EN
	            ;

	// Configure and enable Tx DMA.
	DMA1_CMAR3  = (uint32_t) txbuf;
	DMA1_CPAR3  = (uint32_t) &SPI1_DR;
	DMA1_CNDTR3 = len;
	DMA1_CCR3   = DMA_CCR_DIR
	            | DMA_CCR_MINC
	            | DMA_CCR_MSIZE_8BIT
	            | DMA_CCR_PSIZE_8BIT
	            | DMA_CCR_EN
	            ;

	// Select the chip.
	spi_select();

	// Enable Tx DMA. This will kick off the transaction.
	SPI1_CR2 = SPI_CR2_RXDMAEN
	         | SPI_CR2_TXDMAEN
	         | SPI_CR2_FRXTH
	         ;
}

static void
blocking_transfer (const void *txbuf, const uint32_t len)
{
	start_transfer(txbuf, len);
	while (event_test_and_clear(EVENT_GYRO_RX_FINISHED) == false)
		continue;
}

void
gyro_start_tx (void)
{
	start_transfer(read_fifo, sizeof (read_fifo));
}

static void
init_spi (void)
{
	// Setup PA5 (SPI1_SCK), PA6 (SPI1_MISO) and PA7 (SPI1_MOSI) in AF5.
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO5 | GPIO6 | GPIO7);
	gpio_set_af(GPIOA, GPIO_AF5, GPIO5 | GPIO6 | GPIO7);

	// Start the SPI peripheral.
	SPI1_CR1 = SPI_CR1_MSTR
	         | SPI_CR1_BAUDRATE_FPCLK_DIV_8
	         | SPI_CR1_SSM
	         | SPI_CR1_SSI
	         | SPI_CR1_SPE
	         ;

	// Enable the DMA Rx interrupt.
	nvic_enable_irq(NVIC_DMA1_CHANNEL2_IRQ);
}

static void
init_timer (void)
{
	// Setup PE3 in AF2: TIM3_CH2. TIM3_CH2 is used to supply CS because
	// the LEDs also use GPIOE and clobber the output data register.
	gpio_mode_setup(GPIOE, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3);
	gpio_set_af(GPIOE, GPIO_AF2, GPIO3);

	// Configure TIM3. The settings are not important as long as the timer
	// is running. OC2 is forced high/low manually.
	TIM3_ARR   = 1;
	TIM3_CCMR1 = TIM_CCMR1_OC2M_ACTIVE;
	TIM3_CCER  = TIM_CCER_CC2E;
	TIM3_CR1   = TIM_CR1_CEN;
}

static void
init_exti (void)
{
	// Configure PE1 as an input pin for INT2.
	gpio_mode_setup(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO1);
	exti_select_source(EXTI1, GPIOE);
	exti_set_trigger(EXTI1, EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI1);
	nvic_enable_irq(NVIC_EXTI1_IRQ);
}

void
gyro_config (void)
{
	blocking_transfer(config_fifo, sizeof (config_fifo));
	blocking_transfer(config_ctrl, sizeof (config_ctrl));
}

void
gyro_reboot (void)
{
	init_exti();
	init_timer();
	init_spi();

	blocking_transfer(config_reboot, sizeof (config_reboot));
}
