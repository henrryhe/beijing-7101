#########################################################################
# COPYRIGHT (C) 2006 ST Microelectronics (R&D) Ltd
#
# file: makefile
# Author: gareth-j.powell@st.com, modified from Ralph Kimbers efforts.
#
# Build STPTI memory manager lib
#
#########################################################################

DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
ST20_IMPORTS := stuart sttbx stpio stevt stcommon stos

ST20_TARGETS := memapi.lib 
ST40_TARGETS := libmemapi.a
ST200_TARGETS := libmemapi.a

# local objects which comprise this component
ST20_OBJS := pti_mem.tco pti_list.tco memget.tco pti_hndl.tco 

ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

ifeq ($(findstring STPTI_GPDMA_SUPPORT,$(STPTI_CONFIG)), STPTI_GPDMA_SUPPORT)
  ST20_IMPORTS += stgpdma
endif

ifeq ($(findstring STPTI_FDMA_SUPPORT,$(STPTI_CONFIG)), STPTI_FDMA_SUPPORT)
  ST20_IMPORTS += stfdma
endif

ifeq ($(findstring STPTI_PCP_SUPPORT,$(STPTI_CONFIG)), STPTI_PCP_SUPPORT)
 ST20_IMPORTS += stpcp
endif

ST40_IMPORTS := $(ST20_IMPORTS)
ST200_IMPORTS := $(ST20_IMPORTS)

ifneq ($(findstring pti,$(STPTI_HAL)), pti)
 STPTI_HAL := pti4
endif

# --------------------------------------------------------------------------
# include core,stpti.h, memory manager and tc hal

HEADER_IMPORTS += stpti4 stpti4/src/stpti stpti4/src/stpti/$(STPTI_HAL) stpti4/src/stpti/memapi stpti4/src/stpti/frontend

DVD_CFLAGS += $(STPTI_CONFIG)


# Default rules
include $(DVD_MAKE)/defrules.mak


$($(ARCHITECTURE)_TARGETS):  $($(ARCHITECTURE)_OBJS)
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_OBJS)    
	$(RM) $($(ARCHITECTURE)_TARGETS)

flint:
	$(LINT_COMMAND) *.c *.h



else

include $(DVD_MAKE)/builddir.mak

endif
# --- EOF ---
