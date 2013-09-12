#########################################################################
# COPYRIGHT (C) 2007 ST Microelectronics (R&D) Ltd
#
# file: makefile
# Author: S. Wheatstone
#
# Build STFE HAL lib
#
#########################################################################
DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS += stpti4 stevt stcommon stos

# Other components this component is dependant upon
ST20_IMPORTS := stuart sttbx stpio stevt stcommon stos
EXPORTS :=
ST20_TARGETS := stfe.lib
ST40_TARGETS := libstfe.a
ST200_TARGETS := libstfe.a
# --------------------------------------------------------------------------
# Base objects which comprise this component
ST20_OBJS := stfe.tco

ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
# --------------------------------------------------------------------------

# include core,stpti.h, memory manager and tc hal

HEADER_IMPORTS += stpti4
HEADER_IMPORTS += stpti4/src/stpti stpti4/src/stpti/pti4 stpti4/src/stpti/memapi
HEADER_IMPORTS += stpti4/src/stpti/frontend/stfe stpti4/src/stpti/frontend/tsgdma

DVD_CFLAGS += $(STPTI_CONFIG) -DSTSYS_DEVICE_ACCESS

# --------------------------------------------------------------------------

# Default rules
include $(DVD_MAKE)/defrules.mak

$($(ARCHITECTURE)_TARGETS): $($(ARCHITECTURE)_OBJS)
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_OBJS)
	$(RM) $($(ARCHITECTURE)_TARGETS)

flint:
	$(LINT_COMMAND) *.c *.h

check:
	$(MAKE) -f check.mak



else

include $(DVD_MAKE)/builddir.mak

endif


# --- EOF ---
