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

ST20_TARGETS := pti4.lib
ST40_TARGETS := libpti4.a
ST200_TARGETS := libpti4.a

SUBDIRS = tc

# --------------------------------------------------------------------------
# Base objects which comprise this component
ST20_OBJS := basic.tco extended.tco isr.tco dvb.tco

# Internal 'helpers'
ST20_OBJS += slotlist.tco helper.tco 
ST20_OBJS += session.tco tcinit.tco 

# --------------------------------------------------------------------------

ifneq ($(findstring STPTI_BSL_SUPPORT,$(STPTI_CONFIG)), STPTI_BSL_SUPPORT)
 ST20_OBJS +=  validate.tco exm.tco  config.tco  swcdfifo.tco tcterm.tco
endif

ifeq ($(findstring STPTI_GPDMA_SUPPORT,$(STPTI_CONFIG)), STPTI_GPDMA_SUPPORT)
  ST20_IMPORTS += stgpdma
  ST20_OBJS += gpdma.tco
endif

ifeq ($(findstring STPTI_FDMA_SUPPORT,$(STPTI_CONFIG)), STPTI_FDMA_SUPPORT)
  ST20_IMPORTS += stfdma
endif

ifeq ($(findstring STPTI_PCP_SUPPORT,$(STPTI_CONFIG)), STPTI_PCP_SUPPORT)
 ST20_IMPORTS += stpcp
 ST20_OBJS += pcp.tco
endif

ST40_IMPORTS := $(ST20_IMPORTS) os20emu
ST200_IMPORTS := $(ST20_IMPORTS) os20emu


# --------------------------------------------------------------------------
# Optional components

ifeq ($(findstring STPTI_DC2_SUPPORT,$(STPTI_CONFIG)), STPTI_DC2_SUPPORT)
 ST20_OBJS += dc2.tco
endif

ifneq ($(findstring STPTI_NO_INDEX_SUPPORT,$(STPTI_CONFIG)), STPTI_NO_INDEX_SUPPORT)
 ST20_OBJS += index.tco
endif

ifeq ($(findstring STPTI_CAROUSEL_SUPPORT,$(STPTI_CONFIG)), STPTI_CAROUSEL_SUPPORT)
 ST20_OBJS += carousel.tco
endif

ifeq ($(findstring STPTI_DTV_SUPPORT,$(STPTI_CONFIG)), STPTI_DTV_SUPPORT)
 ST20_OBJS += dtv.tco
endif

# TODO
ifeq ($(findstring STPTI_SP_SUPPORT,$(STPTI_CONFIG)), STPTI_SP_SUPPORT)
 ST20_OBJS += sp.tco
endif

# TODO
ifeq ($(findstring STPTI_DEBUG_SUPPORT,$(STPTI_CONFIG)), STPTI_DEBUG_SUPPORT)
 ST20_OBJS += hal_dbg.tco
endif


ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

# --------------------------------------------------------------------------
# include core,stpti.h, memory manager and tc hal

HEADER_IMPORTS += stpti4 stpti4/src/stpti stpti4/src/stpti/pti4 stpti4/src/stpti/memapi
HEADER_IMPORTS += stpti4/src/stpti/pti4/tc

ifeq ($(DVD_FRONTEND),7200)
HEADER_IMPORTS += stpti4/src/stpti/frontend/stfe
endif

DVD_CFLAGS += $(STPTI_CONFIG) -DSTSYS_DEVICE_ACCESS

# PMC 18/02/03 Adding in define to enable SF CAM arbiter workaround
# workaround replaces h/w arbiter with s/w mutex GNBvd18811
DVD_CFLAGS += -DSTPTI_WORKAROUND_SFCAM


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
