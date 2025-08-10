PROJECT     = stm32f3discovery
DEVICE      = stm32f303vct6
OPENCM3_DIR = lib/libopencm3

CFLAGS  = -std=c11 -O3 -g -flto
CFLAGS += -funsigned-char -fomit-frame-pointer
CFLAGS += -Wall -Wextra -Werror

LDFLAGS = -static -nostartfiles -lc_nano -lnosys -Os -g -flto
LIBNAME = opencm3_stm32f3

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)

# Default to silent mode, run 'make V=1' for a verbose build.
ifneq ($(V),1)
Q := @
MAKEFLAGS += --no-print-directory
endif

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

.PHONY: all clean flash

all: $(PROJECT).bin $(PROJECT).elf

$(OBJS): $(LIBDEPS)

$(LIBDEPS):
	$(Q)$(MAKE) \
	  -C $(OPENCM3_DIR) \
	  TARGETS=stm32/f3 \
	  CFLAGS=-flto

flash: $(PROJECT).bin
	@printf "  FLASH   $^\n";
	$(Q)openocd \
	  -f /usr/share/openocd/scripts/interface/stlink.cfg \
	  -c "transport select hla_swd" \
	  -f /usr/share/openocd/scripts/target/stm32f3x.cfg \
	  -c "init" \
	  -c "reset halt" \
	  -c "stm32f3x unlock 0" \
	  -c "flash write_image erase unlock $(PROJECT).bin 0x08000000" \
	  -c "reset run" \
	  -c "shutdown"

clean:
	$(Q)$(MAKE) -C $(OPENCM3_DIR) clean
	$(Q)$(RM) $(OBJS) $(LDSCRIPT)
	$(Q)$(RM) $(PROJECT).bin $(PROJECT).elf

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
