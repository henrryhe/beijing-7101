DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),7710 5100 5301 5105 5107)"
ST20_IMPORTS := stos sttbx stevt stsys stcommon
else
ST20_IMPORTS := stos sti2c sttbx stevt stsys stcommon
endif

ST40_IMPORTS := $(ST20_IMPORTS)

ST20_EXPORTS := stpccrd.h stpccrd.lib
ST40_EXPORTS := stpccrd.h libstpccrd.a

# local objects which comprise this component
ST20_OBJS :=  stpccrd.tco pccrd_hal.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

ST20_TARGETS := stpccrd.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))

ifdef STPCCRD_DEBUG
CFLAGS += -DSTTBX_PRINT -DSTTBX_REPORT -DSTPCCRD_DEBUG
endif

ifdef STPCCRD_USE_ONE_MODULE
CFLAGS += -DSTPCCRD_USE_ONE_MODULE
endif

# Build options
ifdef STAPI_INTERRUPT_BY_NUMBER
CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER
endif

ifdef STPCCRD_VCC_5V
CFLAGS += -DSTPCCRD_VCC_5V
endif

ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),5514 5516 5517 7100 7710 5100 5301)"
  ifdef STPCCRD_HOT_PLUGIN_ENABLED
	 CFLAGS := $(CFLAGS) -DSTPCCRD_HOT_PLUGIN_ENABLED
  endif
endif
ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),5518 )"
  ifdef STPCCRD_HOT_PLUGIN_ENABLED
     $(error  : STPCCRD_HOT_PLUGIN not supported for 5518 )
  endif
endif
ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),5517)"
 ifdef STPCCRD_SELECT_5517_EMI4
      CFLAGS := $(CFLAGS) -DSTPCCRD_SELECT_5517_EMI4
 endif
endif
ifeq "$(DVD_FRONTEND)" "$(findstring $(DVD_FRONTEND),5514 5518 5528)"
  ifndef STPCCRD_DVBCI_CONTROLLER
   STPCCRD_DVBCI_CONTROLLER := STV0700
  else
   ifneq "$(STPCCRD_DVBCI_CONTROLLER)" "$(findstring $(STPCCRD_DVBCI_CONTROLLER),STV0700 STV0701)"
    $(error  :Unable to identify $(STPCCRD_DVBCI_CONTROLLER))
   endif
  endif
  CFLAGS += -D$(STPCCRD_DVBCI_CONTROLLER)
endif


include $(DVD_MAKE)/defrules.mak

$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)

clean:
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)

# Local dependencies
stpccrd.tco: stpccrd.c stpccrd.h pccrd_hal.h
pccrd_hal.tco: pccrd_hal.c stpccrd.h pccrd_hal.h

else
include $(DVD_MAKE)/builddir.mak
endif
