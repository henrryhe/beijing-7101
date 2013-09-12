DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependent upon
ST20_IMPORTS := stos stboot stcommon stevt sttbx stuart stpio statapi
ST20_OBJS := atapitest.tco
ST20_TARGETS := atapitest.lku

ifdef USE_TESTTOOL
ST20_IMPORTS += testtool
endif

# Use Toolbox Print for test harness output
CFLAGS += -DSTTBX_PRINT -DSTTBX_REPORT

# Options based on platform
ifeq ($(DVD_FRONTEND),5512)
    ST20_EXTRA_LIBS := stfpga.lib
    ST20_EXTRA_OBJS := stfpga.tco
endif

ifeq ($(DVD_FRONTEND),5100)
ifdef ATAPI_CRYPT
    ST20_EXTRA_LIBS := stcrypt.lib
    CFLAGS := $(CFLAGS) -DSTATAPI_CRYPT
endif    
endif

ifeq ($(ARCHITECTURE), ST20)
ifeq ($(DVD_FRONTEND),5514)
    ST20_IMPORTS := $(ST20_IMPORTS) stcfg
endif
ifeq ($(DVD_FRONTEND),5516)
    ST20_IMPORTS := $(ST20_IMPORTS) stcfg
endif
ifeq ($(DVD_FRONTEND),5517)
    ST20_IMPORTS := $(ST20_IMPORTS) stcfg
endif
ifeq ($(DVD_FRONTEND),5528)
    ST20_IMPORTS := $(ST20_IMPORTS) stcfg
endif
endif

# Build options. Hundreds of the things.
ifdef SATA_SUPPORTED
    CFLAGS := $(CFLAGS) -DSATA_SUPPORTED
endif

ifdef ATAPI_FDMA
    CFLAGS += -DATAPI_FDMA
    ST20_IMPORTS += stfdma
endif

ifdef SOAK_TEST
    CFLAGS += -DSOAK_TEST
endif

ifdef ATAPI_BMDMA
    CFLAGS += -DATAPI_BMDMA
endif

ifdef RW_INFO
    CFLAGS += -DRW_INFO
endif

ifdef ATAPI_MEMTEST
    CFLAGS += -DATAPI_MEMTEST
endif

ifdef ATAPI_REGTEST
    CFLAGS += -DATAPI_REGTEST
endif

ifdef STATAPI_OLD_FPGA
    CFLAGS += -DSTATAPI_OLD_FPGA
endif
ifdef ATAPI_DEBUG
    CFLAGS += -DATAPI_DEBUG
endif

ifdef DEBUG
	CFLAGS += -DDEBUG
endif

ifdef HDDI_5514_CUT_3_0
    CFLAGS += -DHDDI_5514_CUT_3_0
endif
ifdef HDDI_5514_CUT_3_1
    CFLAGS := $(CFLAGS) -DHDDI_5514_CUT_3_1
endif

ifdef TEST_TWO_DEVICE
    CFLAGS += -DTEST_TWO_DEVICE
endif

ifdef STATAPI_NO_PARAMETER_CHECK
    CFLAGS := $(CFLAGS) -DSTATAPI_NO_PARAMETER_CHECK
endif

ifdef DISABLE_ICACHE
    CFLAGS := $(CFLAGS) -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
    CFLAGS := $(CFLAGS) -DDISABLE_DCACHE
endif

ifdef STATAPI_SET_EMIREGISTER_MAP
    CFLAGS := $(CFLAGS)  -DSTATAPI_SET_EMIREGISTER_MAP
endif  

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))

ST200_IMPORTS := $(ST20_IMPORTS)
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
ST200_TARGETS := $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))

ifeq ($(ARCHITECTURE),ST200)
LKFLAGS = -Wl,--M $(basename $@).map
endif

ifeq ($(ARCHITECTURE),ST20)
LKFLAGS = -M $(basename $@).map
endif

include $(DVD_MAKE)/defrules.mak

# 5512 only
stfpga.lib: stfpga.tco
	$(BUILD_LIBRARY)

# Executables
#atapitest_LKFLAGS := -fp

$($(ARCHITECTURE)_TARGETS): $($(ARCHITECTURE)_OBJS) $($(ARCHITECTURE)_EXTRA_LIBS) $(IMPORT_LIBS)
	$(LINK_EXECUTABLE)

clean:
	@ $(RM) $($(ARCHITECTURE)_OBJS)
	@ $(RM) $($(ARCHITECTURE)_TARGETS)
	@ $(RM) $($(ARCHITECTURE)_EXTRA_LIBS)
	@ $(RM) $($(ARCHITECTURE)_EXTRA_OBJS)
	@ $(RM) *.map
	@ $(RM) *.dbg

else

include $(DVD_MAKE)/builddir.mak

endif
