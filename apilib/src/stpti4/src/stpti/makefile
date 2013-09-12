#########################################################################
# COPYRIGHT (C) 2006 ST Microelectronics (R&D) Ltd
#
#   file: makefile
# Author: gareth-j.powell@st.com, modified from Ralph Kimbers original efforts.
#
# Build STPTI library & HALs (include TC loader library as well)
#
# 28-5-2003     Update for PTI4 support.        GJP
# 28-7-2003     Refactored PTI4 support         GJP
# 15-9-2003     Ported to new make sys & added
#               ST40/OS21 support               PJW
#########################################################################
DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ST20_TARGETS := stpti.lib
ST40_TARGETS := libstpti.a
ST200_TARGETS := libstpti.a

STPTI_HAL := pti4
SUBDIRS := $(STPTI_HAL) memapi

ifeq ($(DVD_FRONTEND),7200)
SUBDIRS += frontend
endif

# -----------------------IMPORTS------------------------------------------------

HEADER_IMPORTS += stpti4 stpti4/src/stpti/$(STPTI_HAL) stpti4/src/stpti/memapi
HEADER_IMPORTS += stpti4/src/stpti/$(STPTI_HAL)/tc

ST20_IMPORTS := stuart sttbx stpio stevt stcommon stos stsys

ifeq ($(findstring STPTI_GPDMA_SUPPORT,$(STPTI_CONFIG)), STPTI_GPDMA_SUPPORT)
	ST20_IMPORTS += stgpdma
endif

ifeq ($(findstring STPTI_FDMA_SUPPORT,$(STPTI_CONFIG)), STPTI_FDMA_SUPPORT)
	ST20_IMPORTS += stfdma
endif

ifeq ($(findstring STPTI_PCP_SUPPORT,$(STPTI_CONFIG)), STPTI_PCP_SUPPORT)
	ST20_IMPORTS += stpcp
endif

ST40_IMPORTS := $(ST20_IMPORTS) os21emu
ST200_IMPORTS := $(ST20_IMPORTS) os21emu

# -----------------------CFLAGS-------------------------------------------------

DVD_CFLAGS += $(STPTI_CONFIG)

ifeq ($(findstring STPTI_ASSERT_SUPPORT,$(STPTI_CONFIG)), STPTI_ASSERT_SUPPORT)
else
	DVD_CFLAGS += -DNDEBUG
endif

# PMC 18/02/03 Adding in define to enable SF CAM arbiter workaround
# workaround replaces h/w arbiter with s/w mutex GNBvd18811 
# NOTE!  This is still necessary for PTI4L and PTI4SL architectures as
# well as PTI4.
DVD_CFLAGS += -DSTPTI_WORKAROUND_SFCAM

# This define is kept for support for D11 BootLoader (which does not use STSYS_DEVICE_ACCESS)
DVD_CFLAGS += -DSTSYS_DEVICE_ACCESS


# -----------------------OBJECTS TO BUILD---------------------------------------

# Core objects which comprise this component
ST20_OBJS := pti_evt.tco pti_int.tco osx.tco

# API objects which comprise this component
ST20_OBJS += pti_ba.tco pti_fe.tco

# Build options
ifneq ($(findstring STPTI_BSL_SUPPORT,$(STPTI_CONFIG)), STPTI_BSL_SUPPORT)
 ST20_OBJS += pti_vali.tco pti_extd.tco   pti_dvb.tco    pti_swc.tco  
endif

ifneq ($(findstring STPTI_NO_INDEX_SUPPORT,$(STPTI_CONFIG)), STPTI_NO_INDEX_SUPPORT)
 ST20_OBJS += pti_indx.tco
endif

ifeq ($(findstring STPTI_CAROUSEL_SUPPORT,$(STPTI_CONFIG)), STPTI_CAROUSEL_SUPPORT)
 ST20_OBJS += pti_car.tco
endif

ifeq ($(findstring STPTI_DTV_SUPPORT,$(STPTI_CONFIG)), STPTI_DTV_SUPPORT)
 ST20_OBJS += pti_dtv.tco
endif

# TODO
ifeq ($(findstring STPTI_SP_SUPPORT,$(STPTI_CONFIG)), STPTI_SP_SUPPORT)
 ST20_OBJS += pti_sp.tco
endif

# TODO
ifeq ($(findstring STPTI_DEBUG_SUPPORT,$(STPTI_CONFIG)), STPTI_DEBUG_SUPPORT)
 ST20_OBJS += debug.tco
endif

ST40_OBJS :=  $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS :=  $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
# ------------------------------------------------------------------------------

 # Default rules
include $(DVD_MAKE)/defrules.mak

$($(ARCHITECTURE)_TARGETS):  $(SUBDIR_LIBS) $($(ARCHITECTURE)_OBJS)
	$(BUILD_LIBRARY)

FORCE:
    
clean: $(SUBDIR_CLEAN)
	$(RM) $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_OBJS)

else

include $(DVD_MAKE)/builddir.mak

endif

