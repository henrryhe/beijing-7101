#########################################################################
# COPYRIGHT (C) 2003 ST Microelectronics (R&D) Ltd
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

HEADER_IMPORTS += stpti4 stevt stcommon stos

# Other components this component is dependant upon
ST20_IMPORTS := stuart sttbx stpio stevt stcommon stos
EXPORTS :=
ST20_TARGETS := tc.lib
ST40_TARGETS := libtc.a
ST200_TARGETS := libtc.a
# --------------------------------------------------------------------------
# Base objects which comprise this component
ST20_OBJS := tchal.tco

ifeq ($(findstring 5528,$(DVD_FRONTEND)), 5528)
 ST20_OBJS += stdcam.tco
else
 ST20_OBJS += ramcam.tco
endif


ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
# --------------------------------------------------------------------------

# include core,stpti.h, memory manager and tc hal

HEADER_IMPORTS += stpti4
HEADER_IMPORTS += stpti4/src/stpti stpti4/src/stpti/pti4 stpti4/src/stpti/memapi
HEADER_IMPORTS += stpti4/src/stpti/pti4/tc

DVD_CFLAGS += $(STPTI_CONFIG) -DSTSYS_DEVICE_ACCESS

# the -fgcse optimisation turned on by -O2 causes a exception (a compiler optimisation issue?)
# so we suppress that optimsation.  The issue was found in... ramcam.c:TcCam_DTVFilterSetAPG
ramcam_ST40_CFLAGS += -fno-gcse

# PMC 18/02/03 Adding in define to enable SF CAM arbiter workaround
# workaround replaces h/w arbiter with s/w mutex GNBvd18811
DVD_CFLAGS += -DSTPTI_WORKAROUND_SFCAM

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
