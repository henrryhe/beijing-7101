DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# if using os40, add os20emu to header imports
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

# Other components this component is dependant upon
ifneq "$(DVD_OS)" "LINUX"
ifdef DISABLE_TOOLBOX
IMPORTS := stos stcommon stboot stpio
else
IMPORTS := stos stcommon stboot sttbx stuart stpio
endif
else
IMPORTS := stos stcommon stpio 
endif

TARGETS := pio_test$(EXE_SUFFIX)

# ST40
ifndef ST40_TARGETS
ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
ST40_IMPORTS = $(IMPORTS)
endif

# ST200
ifndef ST200_TARGETS
ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
ST200_IMPORTS = $(IMPORTS)
endif

ifndef DISABLE_TOOLBOX
TESTDEFS := -DSTTBX_PRINT
endif

# -DSPEED_TEST_NUM_CALLS - sets number of iterations for benchmark

ifdef SPEED_TEST_NUM_CALLS
TESTDEFS := $(TESTDEFS) -DSPEED_TEST_NUM_CALLS=$(SPEED_TEST_NUM_CALLS)
endif

ifdef DISABLE_ICACHE
    CFLAGS := $(CFLAGS) -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
    CFLAGS := $(CFLAGS) -DDISABLE_DCACHE
endif

CFLAGS := $(CFLAGS) $(TESTDEFS)

ifdef CODETEST
CC = ctcc -v -Xidb=/u/liamw/codetest/test.idb
CFLAGS := $(CFLAGS) -DCODETEST
endif

ifeq ($(ARCHITECTURE),ST200)
LKFLAGS = -Wl,--M $(basename $@).map
endif

ifeq ($(ARCHITECTURE),ST20)
LKFLAGS = -M $(basename $@).map
endif

# Objects
OBJS := pio_test$(OBJ_SUFFIX)

include $(DVD_MAKE)/defrules.mak

$(TARGETS): $(OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $(OBJS)
	@$(RM) *.map
	-$(RM) *.dbg
	

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End
