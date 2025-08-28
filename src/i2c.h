#pragma once

#include <stdint.h>
#include <stddef.h>

extern void i2c_start_xfer (const uint8_t addr,
                            const    void *wr, const size_t wrlen,
                            volatile void *rd, const size_t rdlen);

extern void i2c_blocking_write (const uint8_t addr, const void *wr, const size_t wrlen);
extern void i2c_init (void);
