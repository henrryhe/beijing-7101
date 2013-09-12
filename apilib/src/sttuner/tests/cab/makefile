#  file: sttuner/tests/makefile
#  file: sttuner/tests/makefile
# makes: cab_test.lku version 3.1.0
# history:(d/m/y)
#          8/8/2001 - Modified from 2.0.3 test makefile (bypassed 3.0.0)
#                     to support testing of STTUNER 3.1.0
#
#         22/8/2001 - Added STV0399 support
#
#         10/9/2001 - change name from makefile to cab.mak
#        10/11/2001 - ...and back!
#          8/2/2002 - added mediaref support
#          7/3/2002 - Update to makesystem version 2
#          7/2/2003 - Added STV0297J support
#         30/9/2004 - Modified for OS21
#        20/11/2006 - Added STV0297E support

DVD_MAKE_VERSION := 2



ifeq "$(DVD_OS)" "LINUX"
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak
ifdef USE_STI2C_WRAPPER
HEADER_IMPORTS += stos stcommon stevt sttuner
LINUX_IMPORTS += stos stcommon stevt
else
HEADER_IMPORTS += stos stcommon stevt sti2c sttuner
LINUX_IMPORTS += stos stcommon stevt sti2c
endif
LINUX_TARGETS := cab_test$(EXE_SUFFIX)

# --------------------------------------------------------------------
# Variant flags
CFLAGS += -DSTTUNER_BASIC -DSTTUNER_BASIC_ADV -DSTTUNER_BASIC_ECHO 
# --------------------------------------------------------------------
ifeq "$(TUNER_STANDBY_ON)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_STANDBY_ON
endif

ifeq "$(STTBX_NO_UART)" "YES"
 CFLAGS := $(CFLAGS) -DSTTBX_NO_UART
endif		

ifdef DISABLE_TOOLBOX
 CFLAGS += -DDISABLE_TOOLBOX
endif

ifdef USE_STI2C_WRAPPER
LINUX_IMPORTS = stos stcommon stevt sttuner
else
LINUX_IMPORTS = stos stcommon stevt sti2c sttuner
endif
LINUX_OBJS = cab_test$(OBJ_SUFFIX)
LINUX_TARGETS = cab_test$(EXE_SUFFIX)

# begin environment options ----------------------
#Flag for use with LINUX STI2C kernel or our wrapper
ifeq "$(USE_STI2C_WRAPPER)" "YES"
 CFLAGS := $(CFLAGS) -DUSE_STI2C_WRAPPER
endif


# I2C address for the tuner module if using STV0297/STV0297J/STV0297E (default is0xC0)
ifndef TUNER_I2C
 TUNER_I2C = 0xC0
endif

# I2C baudrate for I2C bus 100k by default
ifndef I2C_BAUDRATE
          I2C_BAUDRATE = 100000
endif

# number of tuners to test
ifndef NUM_TUNER
 NUM_TUNER = 1
endif

# if NUM_TUNER > 1 and TUNER_FIRST defined then tuner #2 is tested first,
# else tuner #1 is tested first.
ifeq "$(TUNER_FIRST)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_FIRST
endif



ifeq "$(DVD_TUNER)" "STV0297"
  CFLAGS := $(CFLAGS) -DTUNER_297
  TEST_DEMOD_TYPE = STV0297
  XTAL_CLOCK = 28920000
  DEMOD_I2C = 0x38
  ifndef TEST_TUNER_TYPE
    TEST_TUNER_TYPE = MT2060
  endif
endif		   

ifeq "$(DVD_TUNER)" "STV0297J"
  CFLAGS := $(CFLAGS) -DTUNER_297J
  TEST_DEMOD_TYPE = STV0297J
  DEMOD_I2C = 0x3A
  XTAL_CLOCK = 25000000
  ifndef TEST_TUNER_TYPE
    TEST_TUNER_TYPE = DCT7040
  endif  	
endif

ifeq "$(DVD_TUNER)" "STV0297E"
  CFLAGS := $(CFLAGS) -DTUNER_297E
  TEST_DEMOD_TYPE = STV0297E
  DEMOD_I2C = 0x38
  XTAL_CLOCK = 27000000
  ifndef TEST_TUNER_TYPE
    TEST_TUNER_TYPE = DCT7045
    endif  	
endif

ifeq "$(DVD_TUNER)" "STB0370QAM"
  CFLAGS := $(CFLAGS) -DTUNER_370QAM
  TEST_DEMOD_TYPE = STB0370QAM
  XTAL_CLOCK = 27000000
  DEMOD_I2C = 0x18
  ifndef TEST_TUNER_TYPE
    TEST_TUNER_TYPE = TD1336
  endif  	
endif		   






# Cable Flag Set
ifeq "$(DVD_STTUNER_USE_CAB)" "YES"
 CFLAGS += -DSTTUNER_USE_CAB
endif



# number of times to do remove/reapply signal feed (antenna disconnection test)
ifndef RELOCK_TEST_NUMTIMES
 RELOCK_TEST_NUMTIMES = 2
endif

# Tuner tracking test number of iterations (default 60 times)
ifndef TUNER_TRACKING
 TUNER_TRACKING = 20
endif

# Tuner TPS extraction test number of iterations 
ifndef TUNER_TPS_COUNTER
 TUNER_TPS_COUNTER = 10
endif


# end environment options ------------------------

# Test definitions required for build of test harness
TESTDEFS =	-DTUNER_TABLE_SIZE=100 -DSTTBX_PRINT -DTUNER_I2C=$(TUNER_I2C) \
		-DDEMOD_I2C=$(DEMOD_I2C) -DI2C_BAUDRATE=$(I2C_BAUDRATE) \
		-DTUNER_TRACKING=$(TUNER_TRACKING) \
		-DTEST_DEMOD_TYPE=STTUNER_DEMOD_$(TEST_DEMOD_TYPE) \
		-DTEST_TUNER_TYPE=STTUNER_TUNER_$(TEST_TUNER_TYPE) \
		-DXTAL_CLOCK=$(XTAL_CLOCK) \
		-DNUM_TUNER=$(NUM_TUNER) -DRELOCK_TEST_NUMTIMES=$(RELOCK_TEST_NUMTIMES) \
		-DSTEVT_ENABLED



ifndef STTUNER_MINIDRIVER
TESTDEFS += -DSTEVT_ENABLED
endif
# Append TESTDEFS
CFLAGS := $(CFLAGS) $(TESTDEFS)

# Default rules
include $(DVD_MAKE)/defrules.mak
ifdef USE_STI2C_WRAPPER
STI2C_IOCTL_LIB := libsti2c.a
STI2C_IOCTL_DIR := ../../../../linux/sti2c_ioctl
LINK_PATH += $(STI2C_IOCTL_DIR)
endif
STTUNER_IOCTL_LIB := libsttuner.a
STTUNER_IOCTL_DIR := ../../../../linux/sttuner_ioctl
LINK_PATH += $(STTUNER_IOCTL_DIR)

#vpath $(STI2C_IOCTL_LIB) $(STI2C_IOCTL_DIR)
ifdef USE_STI2C_WRAPPER
$(STI2C_IOCTL_LIB):
	$(MAKE) -C $(STI2C_IOCTL_DIR)
endif
$(STTUNER_IOCTL_LIB):
	$(MAKE) -C $(STTUNER_IOCTL_DIR)

default: cab_test$(EXE_SUFFIX)
ifdef USE_STI2C_WRAPPER
cab_test$(EXE_SUFFIX): $(LINUX_OBJS) $(IMPORT_LIBS) $(STI2C_IOCTL_LIB) $(STTUNER_IOCTL_LIB)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)
else
cab_test$(EXE_SUFFIX): $(LINUX_OBJS) $(IMPORT_LIBS) $(STTUNER_IOCTL_LIB)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)
endif
clean:

	$(RM) *.a


	$(RM) *.exe
	$(RM) *.o
	$(RM) *.dbg
	$(RM) *.map
	$(RM) *.log


else
include $(DVD_MAKE)/builddir.mak
endif
else #corresponds to -> ifeq "$(DVD_OS)" "LINUX"      Corresponding endif just before EOF

#######################################################################
#----------------------------- OS20/OS21 -----------------------------#
#######################################################################

# Pick up standard variables
include $(DVD_MAKE)/generic.mak

ifeq "$(STTBX_NO_UART)" "YES"
 CFLAGS := $(CFLAGS) -DSTTBX_NO_UART
endif
ifdef DISABLE_TOOLBOX
 CFLAGS += -DDISABLE_TOOLBOX
endif

ifndef DISABLE_TOOLBOX
 IMPORTS = stos stcommon stboot stpio sttbx sti2c stevt sttuner
 ST40_IMPORTS = stos stcommon stboot stpio sttbx sti2c stevt sttuner
 ST200_IMPORTS = stos stcommon stboot stpio sttbx sti2c stevt sttuner
 ifndef STTBX_NO_UART
  IMPORTS += stuart
  ST40_IMPORTS += stuart
  ST200_IMPORTS += stuart
 endif
else
 IMPORTS = stos stcommon stboot stpio sti2c stevt sttuner
 ST40_IMPORTS = stos stcommon stboot stpio sti2c stevt sttuner
 ST200_IMPORTS = stos stcommon stboot stpio sti2c stevt sttuner
endif

ST20_TARGETS = cab_test.lku


ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST200_TARGETS = $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))

# begin environment options ----------------------
ifdef DISABLE_TOOLBOX
CFLAGS := $(CFLAGS) -DDISABLE_TOOLBOX
endif



# tuner type
ifeq "$(DVD_TUNER)" "STV0297J"
  CFLAGS := $(CFLAGS) -DTUNER_297J
  TEST_DEMOD_TYPE = STV0297J
  DEMOD_I2C = 0x3A
  XTAL_CLOCK = 25000000
  ifndef TEST_TUNER_TYPE
    TEST_TUNER_TYPE = DCT7040
  endif
endif

ifeq "$(DVD_TUNER)" "STV0297E"
  CFLAGS := $(CFLAGS) -DTUNER_297E
  TEST_DEMOD_TYPE = STV0297E
  DEMOD_I2C = 0x38
  XTAL_CLOCK = 27000000
  ifndef TEST_TUNER_TYPE
    TEST_TUNER_TYPE = DCT7045
  endif
endif

ifeq "$(DVD_TUNER)" "STV0297"
  CFLAGS := $(CFLAGS) -DTUNER_297
  TEST_DEMOD_TYPE = STV0297
  XTAL_CLOCK = 28920000
  DEMOD_I2C = 0x38
  ifndef TEST_TUNER_TYPE
    TEST_TUNER_TYPE = DCT7040
  endif
endif
ifeq "$(DVD_TUNER)" "STB0370QAM"
  CFLAGS := $(CFLAGS) -DTUNER_370QAM
  TEST_DEMOD_TYPE = STB0370QAM
  XTAL_CLOCK = 27000000
  DEMOD_I2C = 0x18
  ifndef TEST_TUNER_TYPE
    TEST_TUNER_TYPE = TD1336
  endif
endif

ifeq "$(QAMi5107)" "YES"
CFLAGS := $(CFLAGS) -DQAMi5107
endif


# default I2C address for the tuner module
ifndef TUNER_I2C
 TUNER_I2C = 0xC0
endif


# Satellite Flag Set 
ifeq "$(DVD_STTUNER_USE_SAT)" "YES"
 CFLAGS += -DSTTUNER_USE_SAT
endif 

ifeq "$(DVD_STTUNER_DRV_SAT_SCR)" "YES"
CFLAGS += -DSTTUNER_DRV_SAT_SCR
endif

ifeq "$(DVD_STTUNER_DRV_SAT_DISEQC2_SWDECODE_VIA_PIO)" "YES"
 CFLAGS += -DSTTUNER_DISEQC2_SWDECODE_VIA_PIO
 CFLAGS += -DSTTUNER_DISEQC2_SW_DECODE_VIA_PIO
endif 

ifeq "$(DVD_STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND)" "YES"
 CFLAGS += -DSTTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
endif

ifeq "$(DVD_STTUNER_DRV_SAT_LNB21)" "YES"
CFLAGS += -DSTTUNER_DRV_SAT_LNB21
endif

ifeq "$(DVD_STTUNER_DRV_SAT_LNBH21)" "YES"
CFLAGS += -DSTTUNER_DRV_SAT_LNBH21
endif

# Terrestrial Flag Set 
ifeq "$(DVD_STTUNER_USE_TER)" "YES"
 CFLAGS += -DSTTUNER_USE_TER
endif 

# Cable Flag Set
ifeq "$(DVD_STTUNER_USE_CAB)" "YES"
 CFLAGS += -DSTTUNER_USE_CAB
endif

# ATSC VSB Flag Set 
ifeq "$(DVD_STTUNER_USE_ATSC_VSB)" "YES"
 CFLAGS += -DSTTUNER_USE_ATSC_VSB
endif 

# I2C baudrate for I2C bus 100k by default
ifndef I2C_BAUDRATE
 I2C_BAUDRATE = 100000
endif

# number of tuners to test
ifndef NUM_TUNER
 NUM_TUNER = 1
endif

ifeq "$(DVD_STTUNER_BLOCK_SYNC_MODE_SELECT)" "YES"
 CFLAGS := $(CFLAGS) -DSTTUNER_BLOCK_SYNC_MODE_SELECT
endif

# if NUM_TUNER > 1 and TUNER_FIRST defined then tuner #2 is tested first,
# else tuner #1 is tested first.
ifeq "$(TUNER_FIRST)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_FIRST
endif

# number of times to do remove/reapply signal feed (antenna disconnection test)
ifndef RELOCK_TEST_NUMTIMES
 RELOCK_TEST_NUMTIMES = 2
endif

# Tuner tracking test number of iterations (default 60 times)
ifndef TUNER_TRACKING
 TUNER_TRACKING = 20
endif

# Stacked LNB
ifeq "$(USA_STACK_LNB)" "YES"
 CFLAGS := $(CFLAGS) -DUSA_STACK_LNB 
endif

# don't use C++ style comments ( '//' )
ifneq "$(DVD_OS)" "OS21"
ifneq "$(DVD_STTUNER_NOCPPCOMMENTS)" "YES"
 CFLAGS += -cpp
endif
endif

# variant flags
CFLAGS += -DSTTUNER_BASIC -DSTTUNER_BASIC_ADV -DSTTUNER_BASIC_ECHO 
# end environment options ------------------------

# STi5508 I2C bug means we need SW start/stop version of i2c driver
ifeq ($(DVD_FRONTEND), 5508)
 export DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP
endif

# software START/STOP is also supported on 5518
ifeq ($(DVD_FRONTEND),5518)
# export DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP
  # Unified Memory on 5518 -> Set blocksize to 512Kbit for Data Cache
  ifdef UNIFIED_MEMORY
    export DVD_CFLAGS := $(DVD_CFLAGS) -DSTBOOT_DCACHE_REGION0_512KBIT
  endif
endif

# Produce a map file for ST20 [must use '=' not ':=']
ifeq ($(ARCHITECTURE),ST20)
LKFLAGS = -M $(basename $@).map
endif
ifeq ($(ARCHITECTURE),ST40)
LKFLAGS = -Xlinker -Map=$(basename $@).map
endif
ifeq ($(ARCHITECTURE),ST200)
LKFLAGS = -Wl,--M $(basename $@).map
endif

# Test definitions required for build of test harness
TESTDEFS =	-DTUNER_TABLE_SIZE=100 -DSTTBX_PRINT -DTUNER_I2C=$(TUNER_I2C) \
		-DDEMOD_I2C=$(DEMOD_I2C) -DI2C_BAUDRATE=$(I2C_BAUDRATE) \
		-DTUNER_TRACKING=$(TUNER_TRACKING) \
		-DTEST_DEMOD_TYPE=STTUNER_DEMOD_$(TEST_DEMOD_TYPE) \
		-DTEST_TUNER_TYPE=STTUNER_TUNER_$(TEST_TUNER_TYPE) \
		-DXTAL_CLOCK=$(XTAL_CLOCK) \
		-DNUM_TUNER=$(NUM_TUNER) -DRELOCK_TEST_NUMTIMES=$(RELOCK_TEST_NUMTIMES) \
		-DSTEVT_ENABLED

# Append TESTDEFS
CFLAGS := $(CFLAGS) $(TESTDEFS)

# Objects to build
OBJS = cab_test.tco
ST20_OBJS = $(OBJS)
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(OBJS))
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(OBJS))

# Default rules
include $(DVD_MAKE)/defrules.mak

cab_test_INIT := board_init

cab_test$(EXE_SUFFIX): $($(ARCHITECTURE)_OBJS) $(IMPORT_LIBS)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)
	@-$(MKDIR) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@-$(CP) *.map objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@-$(CP) *.dbg objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@$(CP) *$(EXE_SUFFIX) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)

#log test output to file
runlog:
	$(MAKE) run DVD_RUNARGS="-log cab.log"

clean:
	$(RM) *.lib
	$(RM) *.a
	$(RM) *.lku
	$(RM) *.tco
	$(RM) *.exe
	$(RM) *.o
	$(RM) *.dbg
	$(RM) *.map
	$(RM) *.log
	-$(FULLRM) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	-$(RMDIR) objs


endif
#eof

