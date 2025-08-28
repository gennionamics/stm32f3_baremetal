#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>

#include "acc_mag.h"
#include "i2c.h"
#include "event.h"

#define I2C_ADDR_ACC			0x19
#define I2C_ADDR_MAG			0x1E

#define REG_AUTOINC			0x80

#define TEMP_CFG_REG_A_ENABLE		0b11000000

#define CTRL_REG1_A_ODR_NORMAL_1344	0b10010000
#define CTRL_REG1_A_ODR_NORMAL_10	0b00100000
#define CTRL_REG1_A_LPEN		0b00001000
#define CTRL_REG1_A_ZEN			0b00000100
#define CTRL_REG1_A_YEN			0b00000010
#define CTRL_REG1_A_XEN			0b00000001

#define CTRL_REG3_A_I1_DRDY1		0b00010000
#define CTRL_REG3_A_I1_DRDY2		0b00001000
#define CTRL_REG3_A_I1_WTM		0b00000100

#define CTRL_REG4_A_BDU			0b10000000
#define CTRL_REG4_A_FS_2G		0b00000000
#define CTRL_REG4_A_FS_4G		0b00010000
#define CTRL_REG4_A_FS_8G		0b00100000
#define CTRL_REG4_A_FS_16G		0b00110000
#define CTRL_REG4_A_HR			0b00001000

#define CTRL_REG5_A_BOOT		0b10000000
#define CTRL_REG5_A_FIFO_EN		0b01000000
#define CTRL_REG5_A_LIR_INT1		0b00001000
#define CTRL_REG5_A_D4D_INT1		0b00000100
#define CTRL_REG5_A_LIR_INT2		0b00000010
#define CTRL_REG5_A_D4D_INT2		0b00000001

#define CTRL_REG6_A_I2_CLICKEN		0b10000000
#define CTRL_REG6_A_I2_INT1		0b01000000
#define CTRL_REG6_A_I2_INT2		0b00100000
#define CTRL_REG6_A_BOOT_I1		0b00010000
#define CTRL_REG6_A_P2_ACT		0b00001000
#define CTRL_REG6_A_H_LACTIVE		0b00000010

#define FIFO_CTRL_REG_A_STREAM		0b10000000

#define CFG_REG_A_M_REBOOT		0b01000000
#define CFG_REG_A_M_SOFT_RST		0b00100000
#define CFG_REG_A_M_ODR_10		0b00000000
#define CFG_REG_A_M_ODR_20		0b00000100
#define CFG_REG_A_M_ODR_50		0b00001000
#define CFG_REG_A_M_ODR_100		0b00001100

#define CFG_REG_C_M_INT_MAG_PIN		0b01000000
#define CFG_REG_C_M_BDU			0b00010000
#define CFG_REG_C_M_SELF_TEST		0b00000010
#define CFG_REG_C_M_INT_MAG		0b00000001

#define INT_CTRL_REG_M_XIEN		0b10000000
#define INT_CTRL_REG_M_YIEN		0b01000000
#define INT_CTRL_REG_M_ZIEN		0b00100000
#define INT_CTRL_REG_M_IEA		0b00000100
#define INT_CTRL_REG_M_IEN		0b00000001

// Register address enumeration.
enum {
	REG_OUT_TEMP_L_A = 0x0C,
	REG_OUT_TEMP_H_A,
	REG_INT_COUNTER_REG_A,
	REG_WHO_AM_I_A,
	REG_TEMP_CFG_REG_A = 0x1F,
	REG_CTRL_REG1_A,
	REG_CTRL_REG2_A,
	REG_CTRL_REG3_A,
	REG_CTRL_REG4_A,
	REG_CTRL_REG5_A,
	REG_CTRL_REG6_A,
	REG_REFERENCE_A,
	REG_STATUS_REG_A,
	REG_OUT_X_L_A,
	REG_OUT_X_H_A,
	REG_OUT_Y_L_A,
	REG_OUT_Y_H_A,
	REG_OUT_Z_L_A,
	REG_OUT_Z_H_A,
	REG_FIFO_CTRL_REG_A,
	REG_FIFO_SRC_REG_A,
	REG_INT1_CFG_A,
	REG_INT1_SRC_A,
	REG_INT1_THS_A,
	REG_INT1_DURATION_A,
	REG_INT2_CFG_A,
	REG_INT2_SRC_A,
	REG_INT2_THS_A,
	REG_INT2_DURATION_A,
	REG_CLICK_CFG_A,
	REG_CLICK_SRC_A,
	REG_CLICK_THS_A,
	REG_TIME_LIMIT_A,
	REG_TIME_LATENCY_A,
	REG_TIME_WINDOW_A,
	REG_OFFSET_X_REG_L_M = 0x45,
	REG_OFFSET_X_REG_H_M,
	REG_OFFSET_Y_REG_L_M,
	REG_OFFSET_Y_REG_H_M,
	REG_OFFSET_Z_REG_L_M,
	REG_OFFSET_Z_REG_H_M,
	REG_WHO_AM_I_M = 0x4F,
	REG_CFG_REG_A_M = 0x60,
	REG_CFG_REG_B_M,
	REG_CFG_REG_C_M,
	REG_INT_CRTL_REG_M,
	REG_INT_SOURCE_REG_M,
	REG_INT_THS_L_REG_M,
	REG_INT_THS_H_REG_M,
	REG_STATUS_REG_M,
	REG_OUTX_L_REG_M,
	REG_OUTX_H_REG_M,
	REG_OUTY_L_REG_M,
	REG_OUTY_H_REG_M,
	REG_OUTZ_L_REG_M,
	REG_OUTZ_H_REG_M,
};

static struct {
	struct acc acc;
	struct mag mag;
	int16_t temp;
} rxbuf;

static enum State {
	STATE_IDLE,
	STATE_RECV_ACC,
	STATE_DONE_ACC,
	STATE_RECV_TEMP,
	STATE_DONE_TEMP,
	STATE_RECV_MAG,
	STATE_DONE_MAG,
} state;

static const uint8_t config_a_reboot[] = {
	REG_CTRL_REG5_A,
	CTRL_REG5_A_BOOT,
};

static const uint8_t config_a_fifo[] = {
	REG_FIFO_CTRL_REG_A,
	FIFO_CTRL_REG_A_STREAM | ACC_FIFO_SIZE,
};

// Accelerometer control register configuration sequence.
static const uint8_t config_a_ctrl[] = {
	REG_TEMP_CFG_REG_A | REG_AUTOINC,
	TEMP_CFG_REG_A_ENABLE,
	CTRL_REG1_A_ODR_NORMAL_1344 | CTRL_REG1_A_ZEN | CTRL_REG1_A_YEN | CTRL_REG1_A_XEN,
	0b00000000,
	CTRL_REG3_A_I1_WTM,
	CTRL_REG4_A_BDU | CTRL_REG4_A_FS_4G,
	CTRL_REG5_A_FIFO_EN,
};

// Magnetometer reboot configuration sequence.
static const uint8_t config_m_reboot[] = {
	REG_CFG_REG_A_M,
	CFG_REG_A_M_REBOOT,
};

// Magnetometer control register configuration sequence.
static const uint8_t config_m_ctrl[] = {
	REG_CFG_REG_A_M | REG_AUTOINC,
	CFG_REG_A_M_ODR_100,
	0b00000000,
	CFG_REG_C_M_BDU | CFG_REG_C_M_INT_MAG,
};

void
exti2_tsc_isr (void)
{
	exti_reset_request(EXTI2);
	event_raise(EVENT_MAG_DATA_READY);
}

void
exti4_isr (void)
{
	exti_reset_request(EXTI4);
	event_raise(EVENT_ACC_DATA_READY);
}

const struct acc *
acc_mag_get_acc (void)
{
	return &rxbuf.acc;
}

const struct mag *
acc_mag_get_mag (void)
{
	return &rxbuf.mag;
}

const int16_t *
acc_mag_get_temp (void)
{
	return &rxbuf.temp;
}

void
acc_start_tx (void)
{
	static const uint8_t cmd = REG_OUT_X_L_A | REG_AUTOINC;

	i2c_start_xfer(I2C_ADDR_ACC, &cmd, 1, &rxbuf.acc, sizeof (rxbuf.acc));
}

void
temp_start_tx (void)
{
	static const uint8_t cmd = REG_OUT_TEMP_L_A | REG_AUTOINC;

	i2c_start_xfer(I2C_ADDR_ACC, &cmd, 1, &rxbuf.temp, sizeof (rxbuf.temp));
}

void
mag_start_tx (void)
{
	static const uint8_t cmd = REG_OUTX_L_REG_M | REG_AUTOINC;

	i2c_start_xfer(I2C_ADDR_MAG, &cmd, 1, &rxbuf.mag, sizeof (rxbuf.mag));
}

void
acc_mag_step (void)
{
	static const enum State next[] = {
		[STATE_RECV_ACC]  = STATE_DONE_ACC,
		[STATE_RECV_MAG]  = STATE_DONE_MAG,
		[STATE_RECV_TEMP] = STATE_DONE_TEMP,
	};

	for (;;)
	{
		switch (state)
		{
		case STATE_RECV_ACC:
		case STATE_RECV_MAG:
		case STATE_RECV_TEMP:

			// Do nothing while the transfers are ongoing.
			if (event_test_and_clear(EVENT_I2C_FINISHED) == false)
				return;

			// Enter one of the DONE states.
			state = next[state];
			break;

		case STATE_DONE_ACC:

			// After receiving ACC, also receive TEMP.
			event_raise(EVENT_ACC_FINISHED);
			state = STATE_RECV_TEMP;
			temp_start_tx();
			return;

		case STATE_DONE_MAG:
			event_raise(EVENT_MAG_FINISHED);
			state = STATE_IDLE;
			continue;

		case STATE_DONE_TEMP:
			event_raise(EVENT_ACC_TEMP_FINISHED);
			state = STATE_IDLE;
			continue;

		case STATE_IDLE:
			if (event_test_and_clear(EVENT_ACC_DATA_READY)) {
				state = STATE_RECV_ACC;
				acc_start_tx();
			}
			else if (event_test_and_clear(EVENT_MAG_DATA_READY)) {
				state = STATE_RECV_MAG;
				mag_start_tx();
			}
			return;
		}

		// Executed before going to a DONE state. Check if the data
		// ready pin went down after reading from the sensor. If it is
		// still high, it is possible that the FIFO is still above the
		// watermark level and needs more reads. Fire events manually.
		if (gpio_get(GPIOE, GPIO2) != 0)
			event_raise(EVENT_MAG_DATA_READY);

		if (gpio_get(GPIOE, GPIO4) != 0)
			event_raise(EVENT_ACC_DATA_READY);
	}
}

void
acc_mag_config (void)
{
	i2c_blocking_write(I2C_ADDR_ACC, config_a_ctrl, sizeof (config_a_ctrl));
	i2c_blocking_write(I2C_ADDR_ACC, config_a_fifo, sizeof (config_a_fifo));
	i2c_blocking_write(I2C_ADDR_MAG, config_m_ctrl, sizeof (config_m_ctrl));
}

void
acc_mag_reboot (void)
{
	// Configure PE4 as an input pin for accelerometer INT1.
	gpio_mode_setup(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO4);
	exti_select_source(EXTI4, GPIOE);
	exti_set_trigger(EXTI4, EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI4);
	nvic_enable_irq(NVIC_EXTI4_IRQ);

	// Configure PE2 as an input pin for magnetometer DRDY.
	gpio_mode_setup(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, GPIO2);
	exti_select_source(EXTI2, GPIOE);
	exti_set_trigger(EXTI2, EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI2);
	nvic_enable_irq(NVIC_EXTI2_TSC_IRQ);

	// Issue the reboot commands.
	i2c_blocking_write(I2C_ADDR_ACC, config_a_reboot, sizeof (config_a_reboot));
	i2c_blocking_write(I2C_ADDR_MAG, config_m_reboot, sizeof (config_m_reboot));
}
