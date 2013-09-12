DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ifeq ($(strip $(CLKRV_TESTING)),1)
	CFLAGS := $(CFLAGS) -DCLKRV_TESTING
endif

# Build options
ifdef STCLKRV_FILTER_TASK_PRIORITY
CFLAGS += -DSTCLKRV_FILTER_TASK_PRIORITY=$(STCLKRV_FILTER_TASK_PRIORITY)
endif
#CFLAGS += -DSTTBX_PRINT

ifndef STCLKRV_CRYSTAL_ERROR
STCLKRV_CRYSTAL_ERROR=0x00
endif

CFLAGS += -DSTCLKRV_CRYSTAL_ERROR=$(STCLKRV_CRYSTAL_ERROR)

ifndef STCLKRV_EXT_CLK_MHZ
STCLKRV_EXT_CLK_MHZ=0x00
endif

ifeq ($(findstring -DSTCLKRV_EXT_CLK_MHZ,$(CFLAGS)),)
CFLAGS += -DSTCLKRV_EXT_CLK_MHZ=$(STCLKRV_EXT_CLK_MHZ)
endif

ifdef STCLKRV_FILTER_STACK_SIZE
CFLAGS += -DSTCLKRV_FILTER_STACK_SIZE=$(STCLKRV_FILTER_STACK_SIZE)
endif

ifdef GRADUAL_CORRECTION_FACTOR
CFLAGS += -DGRADUAL_CORRECTION_FACTOR=$(GRADUAL_CORRECTION_FACTOR)
endif

ifdef STCLKRV_INTERRUPT_TIME_SPAN
CFLAGS += -DSTCLKRV_INTERRUPT_TIME_SPAN=$(STCLKRV_INTERRUPT_TIME_SPAN)
endif

ifdef STCLKRV_NO_PTI
CFLAGS += -DSTCLKRV_NO_PTI
endif

ifdef DISABLE_TOOLBOX 
CFLAGS += -DSTCLKRV_NO_TBX
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER
endif

# Check if HDMI audio workaround enabled
ifdef WA_GNBvd44290
CFLAGS += -DWA_GNBvd44290
endif

ifdef WA_GNBvd54182
CFLAGS += -DWA_GNBvd54182
endif

ifdef WA_GNBvd56512
CFLAGS += -DWA_GNBvd56512
endif


# Other components this component is dependant upon
HEADER_IMPORTS :=
ifndef STCLKRV_NO_PTI

# stpti4 lite for 7710/5105 and stpti4 for rest

ifeq "$(findstring $(DVD_FRONTEND),5105 7710 5107)" "$(DVD_FRONTEND)"
    ST20_IMPORTS := stpti4lt stos stevt sttbx
else
ifeq "$(findstring $(DVD_FRONTEND),5188)" "$(DVD_FRONTEND)"
    ST20_IMPORTS := stdemux stfdma stos stevt sttbx
else    
    ST20_IMPORTS := $(DVD_TRANSPORT) stos stevt sttbx
endif
endif

else
ST20_IMPORTS := stos stevt sttbx
endif

ST20_EXPORTS := stclkrv.h stclkrv.lib

# Default build targets
ST20_TARGETS = stclkrv.lib

ifeq ($(strip $(CLKRV_TESTING)),1)
CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS) -DCLKRV_FILTERDEBUG
else
CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)
endif

# Setup STPTI defines for DTV
ifeq "$(DVD_SERVICE)" "DIRECTV"
  CFLAGS += -DSTPTI_DTV_SUPPORT
endif

ifeq ($(strip $(CLKRV_TESTING)),1)
# local objects which comprise this component
ifdef STCLKRV_NO_PTI
ST20_OBJS := stclkrv.tco clkrvfilter.tco
else
ST20_OBJS := stclkrv.tco clkrvfilter.tco clkrvsim.tco
endif
else
ST20_OBJS := stclkrv.tco clkrvfilter.tco
endif

# Then set up the ST40 versions of all those. Flags will probably be the
# same, so we don't touch those.
ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

ST200_IMPORTS := $(ST20_IMPORTS)
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
include $(DVD_MAKE)/defrules.mak

# This builds this component
$($(ARCHITECTURE)_TARGETS): $($(ARCHITECTURE)_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

#clean:
#	@$(ECHO) Cleaning $(TARGETS)
#	-$(RM) $(OBJS)
#	-$(RM) $(TARGETS)
#	-$(RM) *.tco
#	-$(RM) *.lib
clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)	
else

include $(DVD_MAKE)/builddir.mak

endif






