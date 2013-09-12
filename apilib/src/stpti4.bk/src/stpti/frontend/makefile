#########################################################################
# COPYRIGHT (C) 2006 ST Microelectronics (R&D) Ltd
#
# file: makefile
# Author: gareth-j.powell@st.com, modified from Ralph Kimbers efforts.
#
# Build PTI HAL lib for use by STPTI4 library
#
#########################################################################
DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
ST20_IMPORTS := stuart sttbx stpio stevt stcommon stos

ST20_TARGETS := frontend.lib
ST40_TARGETS := libfrontend.a
ST200_TARGETS := libfrontend.a

SUBDIRS = stfe tsgdma

# --------------------------------------------------------------------------
# Base objects which comprise this component
ST20_OBJS := frontend.tco

# --------------------------------------------------------------------------

ST40_IMPORTS := $(ST20_IMPORTS) os20emu
ST200_IMPORTS := $(ST20_IMPORTS) os20emu


ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

# --------------------------------------------------------------------------

HEADER_IMPORTS += stpti4 stpti4/src/stpti stpti4/src/stpti/pti4 stpti4/src/stpti/memapi
HEADER_IMPORTS += stpti4/src/stpti/frontend/stfe stpti4/src/stpti/frontend/tsgdma

DVD_CFLAGS += $(STPTI_CONFIG) -DSTSYS_DEVICE_ACCESS


# --------------------------------------------------------------------------

# Default rules
include $(DVD_MAKE)/defrules.mak

$($(ARCHITECTURE)_TARGETS):  $(SUBDIR_LIBS) $($(ARCHITECTURE)_OBJS)
	$(BUILD_LIBRARY)

clean: $(SUBDIR_CLEAN)
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
