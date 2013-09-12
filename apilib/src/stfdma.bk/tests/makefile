DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ST20_IMPORTS := stfdma stboot sttbx stcommon stpio stos
ST20_TARGETS := fdmatst.lku

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))

ST200_IMPORTS := $(ST20_IMPORTS)
##ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_TARGETS := $(ST40_TARGETS)

## Ensure sttbx built without dependancy on stuart
export DVD_CFLAGS += -DSTTBX_NO_UART

FDMA_CFLAGS := -DSTTBX_PRINT

ifdef STFDMA_TEST_VERBOSE
      FDMA_CFLAGS := $(FDMA_CFLAGS) -DSTFDMA_TEST_VERBOSE
endif

ifdef STFDMA_NO_PARAMETER_CHECK
      FDMA_CFLAGS := $(FDMA_CFLAGS) -DSTFDMA_NO_PARAMETER_CHECK
endif

ifdef STFDMA_NO_USAGE_CHECK
      FDMA_CFLAGS := $(FDMA_CFLAGS) -DSTFDMA_NO_USAGE_CHECK
endif

ifdef STFDMA_DEBUG_SUPPORT
      FDMA_CFLAGS := $(FDMA_CFLAGS) -DSTFDMA_DEBUG_SUPPORT
endif

ifdef STFDMA_NO_PACED_TESTS
      FDMA_CFLAGS := $(FDMA_CFLAGS) -DSTFDMA_NO_PACED_TESTS
endif

ifdef STFDMA_SIM
      FDMA_CFLAGS := $(FDMA_CFLAGS) -DSTFDMA_SIM
      SIM_OBJS := fdmasim.tco
else
      SIM_OBJS :=
endif

ifdef STFDMA_SOAK_DURATION
      FDMA_CFLAGS := $(FDMA_CFLAGS) -DSOAK -DSTFDMA_SOAK_DURATION=$(STFDMA_SOAK_DURATION)
      ST20_OBJS := fdmatst.tco soak.tco $(SIM_OBJS)
else
ifeq "$(filter $(DVD_FRONTEND),7100 7109)" "$(DVD_FRONTEND)"
      ST20_OBJS := fdmatst.tco param.tco func.tco leak.tco state.tco audiopaced.tco $(SIM_OBJS)
else
      ST20_OBJS := fdmatst.tco param.tco func.tco leak.tco state.tco $(SIM_OBJS)
endif
ifdef STFDMA_STCM
SUBDIRS := stcm
CFLAGS += -DSTCM_TEST
endif
endif

ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

ifdef DISABLE_DCACHE
      FDMA_CFLAGS := $(FDMA_CFLAGS) -DDISABLE_DCACHE
endif

ifdef DISABLE_ICACHE
      FDMA_CFLAGS := $(FDMA_CFLAGS) -DDISABLE_ICACHE
endif

CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS) $(TESTFLAGS) $(FDMA_CFLAGS)

ifeq "$(DVD_TOOLSET)" "ST200"
      CFLAGS += -DARCHITECTURE_LX
endif

FDMA_COMPONENT    := $(notdir $(DVD_BUILD_DIR))

PATH_SRC_DIR := $(subst $(GOOD_SLASH)$(FDMA_COMPONENT),,$(DVD_BUILD_DIR))

INCLUDE_PATH := $(PATH_SRC_DIR) $(PATH_SRC_DIR)/tests/verifinc

include $(DVD_MAKE)/defrules.mak

ifeq ($(ARCHITECTURE), ST40)
    LKFLAGS:= -Map=stfdma.map
else
    ifeq ($(ARCHITECTURE), ST20)
        LKFLAGS := -M stfdma.map
    else
        ifeq ($(ARCHITECTURE),ST200)
            LKFLAGS = -Wl,--M $(basename $@).map
        endif
    endif
endif

# This builds this component
fdmatst.out: $(ST200_OBJS) $(IMPORT_LIBS) $(SUBDIR_LIBS)
	      $(LINK_EXECUTABLE)

fdmatst.lku: $(ST20_OBJS) $(IMPORT_LIBS) $(SUBDIR_LIBS)
	      $(LINK_EXECUTABLE)

fdmatst.exe: $(ST40_OBJS) $(IMPORT_LIBS) $(SUBDIR_LIBS)
	      $(LINK_EXECUTABLE)

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

emulate :
	st20run -l /home/damia/orchardd/doc/5100/config -i connect.cfg -t sti5100EMU fdmatst.lku $(DVD_RUNARGS)


#local dependancies
stfdma.tco	:	stfdma.h

else

include $(DVD_MAKE)/builddir.mak

endif
