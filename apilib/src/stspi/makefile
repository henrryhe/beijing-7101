DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
IMPORTS := stpio
EXPORTS := stspi.h $(LIB_PREFIX)stspi$(LIB_SUFFIX)
TARGETS := $(LIB_PREFIX)stspi$(LIB_SUFFIX)

ifndef ST40_TARGETS
ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
ST40_IMPORTS = $(IMPORTS)
endif

ifeq "$(DVD_OS)" "OS40"
ST40_IMPORTS := os20emu
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

# Local objects which comprise this component
OBJS := stspi$(OBJ_SUFFIX)

ifdef STSPI_MASTER_SLAVE
CFLAGS := $(CFLAGS) -DSTSPI_MASTER_SLAVE
endif

ifdef DISABLE_TOOLBOX
CFLAGS := $(CFLAGS) -DSTSPI_NO_TBX
endif

# For I2C and SPI using same SSC
ifdef STI2C_STSPI_SAME_SSC_SUPPORT
	CFLAGS := $(CFLAGS) -DSTI2C_STSPI_SAME_SSC_SUPPORT
endif

include $(DVD_MAKE)/defrules.mak

$(LIB_PREFIX)stspi$(LIB_SUFFIX): $(OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
	
clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)

# Local dependencies
stspi.tco: stspi.c stspi.h


else
include $(DVD_MAKE)/builddir.mak
endif
