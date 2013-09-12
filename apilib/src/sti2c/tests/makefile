DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak
ifneq "$(DVD_OS)" "LINUX"
ifdef DISABLE_TOOLBOX
IMPORTS := sti2c stpio stboot stcommon stevt stos
else
IMPORTS := sti2c stpio stboot stcommon stevt sttbx stuart stos
endif
else
IMPORTS := stos stcommon sti2c
endif

ifeq ($(DVD_FRONTEND),5188)
IMPORTS := $(IMPORTS) stspi
endif

ifneq "$(DVD_OS)" "LINUX"
ifeq ($(DVD_FRONTEND),5188)
TARGETS := i2ctest$(EXE_SUFFIX) i2cmstest$(EXE_SUFFIX) i2cspitest$(EXE_SUFFIX)
else
TARGETS := i2ctest$(EXE_SUFFIX) i2cmstest$(EXE_SUFFIX)
endif
else
TARGETS := i2ctest$(EXE_SUFFIX)
endif

ifdef DISABLE_ICACHE
TEST_FLAGS += -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
TEST_FLAGS +=  -DDISABLE_DCACHE
endif

ifdef DISABLE_TOOLBOX
TEST_FLAGS +=  -DSTSPI_NO_TBX
endif

ifndef ST40_TARGETS
  ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
  ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
  ST40_IMPORTS = $(IMPORTS)
endif

ifndef ST200_TARGETS
  ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
  ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
  ST200_IMPORTS = $(IMPORTS)
endif
# local objects which comprise this component

i2ctest_OBJS := i2ctest$(OBJ_SUFFIX) 
i2cmstest_OBJS := i2cmstest$(OBJ_SUFFIX)
ifeq ($(DVD_FRONTEND),5188)
i2cspitest_OBJS := i2cspitest$(OBJ_SUFFIX)
endif

ifeq ($(DVD_FRONTEND),5188)
all_OBJS := $(i2ctest_OBJS) $(i2cmstest_OBJS) $(i2cspitest_OBJS)
else
all_OBJS := $(i2ctest_OBJS) $(i2cmstest_OBJS)
endif

# STi5508 only has one I2C bus (BACK_BUS)
# whereas STi5510 and ATi5512 have two buses (FRONT_BUS and BACK_BUS)
ifeq ($(DVD_FRONTEND),5510)
    TEST_FLAGS = -DFRONT_BUS_PRESENT
else 
    ifeq ($(DVD_FRONTEND),5512)
        TEST_FLAGS = -DFRONT_BUS_PRESENT
    else 
        ifeq ($(DVD_PLATFORM),mb5518)
            TEST_FLAGS = -DFRONT_BUS_PRESENT
        endif
    endif
endif

# Only software START/STOP is currently supported on 5508
ifeq ($(DVD_FRONTEND),5508)
    export DVD_CFLAGS = -DSTI2C_SW_START_STOP
endif

# software START/STOP is also supported on 5518
ifeq ($(DVD_PLATFORM),mb275)
    export DVD_CFLAGS = -DSTI2C_SW_START_STOP
endif

ifdef TUNER_TEST_EXTENDED
    CFLAGS := $(CFLAGS) -DTUNER_TEST_EXTENDED
endif

ifdef BAUD_RATE_FAST
    CFLAGS := $(CFLAGS) -DBAUD_RATE_FAST
endif

ifdef TUNER_TEST_CYCLES
CFLAGS := $(CFLAGS) -DTUNER_TEST_CYCLES=$(TUNER_TEST_CYCLES)
endif

ifdef TEST_FIFO
    CFLAGS := $(CFLAGS) -DTEST_FIFO
endif

ifdef STI2C_MASTER_ONLY
    CFLAGS := $(CFLAGS) -DSTI2C_MASTER_ONLY
endif

ifdef STI2C_NO_PARAM_CHECK
	CFLAGS := $(CFLAGS) -DSTI2C_NO_PARAM_CHECK
endif

ifdef STI2C_NO_PIO
	CFLAGS := $(CFLAGS) -D STI2C_NO_PIO
endif

ifdef DISPLAY_INIT_CHAIN
    CFLAGS := $(CFLAGS) -DDISPLAY_INIT_CHAIN
endif

ifdef DISPLAY_OPEN_CHAIN
    CFLAGS := $(CFLAGS) -DDISPLAY_OPEN_CHAIN
endif

ifdef STI2C_TRACE
	CFLAGS := $(CFLAGS) -DSTI2C_TRACE
endif

ifdef USE_ONE_BOARD
    CFLAGS := $(CFLAGS) -DUSE_ONE_BOARD
endif

ifdef STI2C_DEBUG
    CFLAGS := $(CFLAGS) -DSTI2C_DEBUG
endif

ifdef DISABLE_ICACHE
    CFLAGS := $(CFLAGS) -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
    CFLAGS := $(CFLAGS) -DDISABLE_DCACHE
endif

ifdef STI2C_DEBUG_BUS_STATE
    CFLAGS := $(CFLAGS) -DSTI2C_DEBUG_BUS_STATE
endif

ifdef DISABLE_TOOLBOX 
    CFLAGS := $(CFLAGS) -DSTI2C_NO_TBX
endif
    
ifdef STI2C_ADAPTIVE_BAUDRATE
    CFLAGS := $(CFLAGS) -DSTI2C_ADAPTIVE_BAUDRATE
endif

# For I2C and SPI using same SSC
ifdef STI2C_STSPI_SAME_SSC_SUPPORT
    CFLAGS := $(CFLAGS) -DSTI2C_STSPI_SAME_SSC_SUPPORT
endif
    
# Use Toolbox Print for test harness output
CFLAGS := $(CFLAGS) $(TEST_FLAGS) -DSTTBX_PRINT -D$(DVD_PLATFORM)

include $(DVD_MAKE)/defrules.mak

# Master mode test harness
i2ctest_INIT := board_init
i2ctest$(EXE_SUFFIX): $(i2ctest_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

# Master/Slave mode test harness
i2cmstest_INIT := board_init
i2cmstest$(EXE_SUFFIX):$(i2cmstest_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

ifeq ($(DVD_FRONTEND),5188)
# Same SSC test harness
i2cspitest_INIT := board_init
i2cspitest$(EXE_SUFFIX): $(i2cspitest_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif
	
clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $(all_OBJS)
	-$(RM) $(TARGETS)
	-$(RM) *.dbg

else
include $(DVD_MAKE)/builddir.mak
endif
