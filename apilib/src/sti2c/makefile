DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
IMPORTS := stpio stos
EXPORTS := sti2c.h $(LIB_PREFIX)sti2c$(LIB_SUFFIX)

TARGETS := $(LIB_PREFIX)sti2c$(LIB_SUFFIX)

ifndef ST40_TARGETS
ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
ST40_IMPORTS = $(IMPORTS)
endif

ifndef ST200_TARGETS
  ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
  ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
  ST200_IMPORTS = $(IMPORTS)
endif

ifeq "$(DVD_OS)" "OS40"
ST40_IMPORTS := os20emu
endif

# local objects which comprise this component
OBJS := sti2c$(OBJ_SUFFIX)

CFLAGS := $(CFLAGS) -DSTTBX_PRINT
ifdef STI2C_TRACE
	CFLAGS := $(CFLAGS) -DSTI2C_TRACE -DSTTBX_PRINT
endif

ifdef DISABLE_TOOLBOX
CFLAGS := $(CFLAGS) -DSTSPI_NO_TBX
endif

ifdef STI2C_DEBUG
	CFLAGS := $(CFLAGS) -DSTI2C_DEBUG -DSTTBX_PRINT
endif

ifdef STI2C_MASTER_ONLY
	CFLAGS := $(CFLAGS) -D STI2C_MASTER_ONLY
endif

ifdef STI2C_NO_PARAM_CHECK
	CFLAGS := $(CFLAGS) -D STI2C_NO_PARAM_CHECK
endif

ifdef STI2C_SLAVE_STRETCH
	CFLAGS := $(CFLAGS) -D STI2C_SLAVE_STRETCH
endif

ifdef STI2C_DEMOD_ACCESSED
	CFLAGS := $(CFLAGS) -D STI2C_DEMOD_ACCESSED
endif

ifdef STI2C_NO_PIO
	CFLAGS := $(CFLAGS) -D STI2C_NO_PIO
endif

ifdef STI2C_LOCK_DELAY
    CFLAGS := $(CFLAGS) -DSTI2C_LOCK_DELAY=$(STI2C_LOCK_DELAY)
endif

ifdef STI2C_ADAPTIVE_BAUDRATE
    CFLAGS := $(CFLAGS) -DSTI2C_ADAPTIVE_BAUDRATE
endif

# For I2C and SPI using same SSC
ifdef STI2C_STSPI_SAME_SSC_SUPPORT
	CFLAGS := $(CFLAGS) -DSTI2C_STSPI_SAME_SSC_SUPPORT
endif    
# When building for use on one board, the PIO pins need to be programmed
# at about the same time. This has to be done via code change inside the
# driver; see STI2C_Init for more comments.
ifdef USE_ONE_BOARD
    CFLAGS := $(CFLAGS) -DUSE_ONE_BOARD
endif

ifdef DISABLE_TOOLBOX 
    CFLAGS := $(CFLAGS) -DSTI2C_NO_TBX
endif

ifdef STI2C_DEBUG_BUS_STATE
    CFLAGS := $(CFLAGS) -DSTI2C_DEBUG_BUS_STATE
endif
# If the SSC suffers from a timing fault in slave mode, activate code to
# work around it.
ifdef STI2C_SLAVE_STRETCH
    CFLAGS := $(CFLAGS) -DSTI2C_SLAVE_STRETCH
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS := $(CFLAGS) -DSTAPI_INTERRUPT_BY_NUMBER
endif

include $(DVD_MAKE)/defrules.mak

$(LIB_PREFIX)sti2c$(LIB_SUFFIX): $(OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $(OBJS)

# Local dependencies
sti2c.tco: sti2c.c sti2c.h

else
include $(DVD_MAKE)/builddir.mak
endif
