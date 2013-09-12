#  file: sttuner/tests/ter/makefile
# makes: ter_test.lku
# history:(d/m/y)
#         12/11/2001 - for terrestrial
#

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

LINUX_TARGETS := ter_test$(EXE_SUFFIX)

# --------------------------------------------------------------------
# Variant flags
CFLAGS += -DSTTUNER_BASIC -DSTTUNER_BASIC_ADV -DSTTUNER_BASIC_ECHO

# --------------------------------------------------------------------

ifeq "$(DVD_STTUNER_MINIDRIVER)" "YES"
   CFLAGS := $(CFLAGS) -DSTTUNER_MINIDRIVER
   STTUNER_MINIDRIVER := 1
endif


ifeq "$(STTBX_NO_UART)" "YES"
 CFLAGS := $(CFLAGS) -DSTTBX_NO_UART
endif
ifdef USE_STI2C_WRAPPER
LINUX_IMPORTS += stos stcommon stevt sttuner
else
LINUX_IMPORTS += stos stcommon stevt sti2c sttuner
endif

LINUX_OBJS = ter_test$(OBJ_SUFFIX)
LINUX_TARGETS = ter_test$(EXE_SUFFIX)

#Flag for use with LINUX STI2C kernel or our wrapper
ifeq "$(USE_STI2C_WRAPPER)" "YES"
 CFLAGS := $(CFLAGS) -DUSE_STI2C_WRAPPER
endif

#check whether 370/372 enabled or not
ifeq "$(TUNER_370VSB)" "YES"
TUNER_37X_ENABLED :=1
endif
ifeq "$(TUNER_372)" "YES"
TUNER_37X_ENABLED :=1
endif

# begin environment options ----------------------

# STV0360/STV0361/SV0362 I2C address for the demodulator (default is 0x38)
ifndef DEMOD_I2C
ifndef TUNER_37X_ENABLED
 DEMOD_I2C_0 = 0x38
 DEMOD_I2C_1 = 0x3C
endif

ifdef TUNER_37X_ENABLED
 DEMOD_I2C_0 = 0x18
 DEMOD_I2C_1 = 0x1C
endif
endif

# I2C address for the tuner module if using STV0360/STV0361 (default is0xC0)
ifndef TUNER_I2C
ifndef TUNER_37X_ENABLED
 TUNER_I2C = 0xC0
endif
ifdef TUNER_37X_ENABLED
ifeq "$(TUNER_370VSB)" "YES"
TUNER_I2C = 0xC2
endif
ifeq "$(TUNER_372)" "YES"
TUNER_I2C = 0xC2
endif
endif
endif

# I2C baudrate for I2C bus 100k by default
ifndef I2C_BAUDRATE
          I2C_BAUDRATE = 100000
   #      I2C_BAUDRATE = 90000
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

# if TUNER_360 defined then the tuner type tested is set to STV0360
ifeq "$(TUNER_360)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_360
endif

# if TUNER_361 defined then the tuner type tested is set to STV0361
ifeq "$(TUNER_361)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_361
endif

# if TUNER_362 defined then the tuner type tested is set to STV0362
ifeq "$(TUNER_362)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_362
endif

# To make TPS_CELLID_TEST ON
ifeq "$(TPS_CELLID_TEST)" "YES"
 CFLAGS := $(CFLAGS) -DTPS_CELLID_TEST
endif

ifeq "$(TUNER_370VSB)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_370VSB
endif
ifeq "$(TUNER_372)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_372
endif

# Testing with DTT Transmission (Noida setup only)
ifeq "$(TEST_NOIDA_DTT_TRANSMISSION)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_NOIDA_DTT_TRANSMISSION
endif
ifeq "$(TEST_CHANNEL_ZAP)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_CHANNEL_ZAP
endif
ifeq "$(TEST_SET_30MHZ_REG_SET)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_SET_30MHZ_REG_SET
endif
ifeq "$(TEST_HIERARCHY)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_HIERARCHY
endif
ifeq "$(TEST_CONFIGURE_IF_IQMODE)" "YES"
CFLAGS := $(CFLAGS) -DTEST_CONFIGURE_IF_IQMODE
endif
# RFLevel monitoring 
ifeq "$(DVD_STTUNER_RFLEVEL_MONITOR)" "YES"
 CFLAGS += -DSTTUNER_RF_LEVEL_MONITOR
endif

# Terrestrial Flag Set
ifeq "$(DVD_STTUNER_USE_TER)" "YES"
 CFLAGS += -DSTTUNER_USE_TER
endif

# Terrestrial Flag Set
ifeq "$(DVD_STTUNER_USE_SAT)" "YES"
 CFLAGS += -DSTTUNER_USE_SAT
endif
# Terrestrial Flag Set
ifeq "$(DVD_STTUNER_USE_CAB)" "YES"
 CFLAGS += -DSTTUNER_USE_CAB
endif

ifeq "$(DVD_STTUNER_USE_ATSC_VSB)" "YES"
 CFLAGS += -DSTTUNER_USE_ATSC_VSB
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

# Tuner type default DTT7572
ifndef TEST_TUNER_TYPE
 TEST_TUNER_TYPE = DTT7592
endif

# end environment options ------------------------

# Test definitions required for build of test harness
TESTDEFS = -DTUNER_TABLE_SIZE=100 -DSTTBX_PRINT -DTUNER_I2C=$(TUNER_I2C) \
           -DDEMOD_I2C_0=$(DEMOD_I2C_0) -DDEMOD_I2C_1=$(DEMOD_I2C_1) -DI2C_BAUDRATE=$(I2C_BAUDRATE) \
           -DTUNER_TRACKING=$(TUNER_TRACKING) \
           -DTUNER_TPS_COUNTER=$(TUNER_TPS_COUNTER) \
           -DTEST_TUNER_TYPE=STTUNER_TUNER_$(TEST_TUNER_TYPE) \
           -DNUM_TUNER=$(NUM_TUNER) -DRELOCK_TEST_NUMTIMES=$(RELOCK_TEST_NUMTIMES)
           

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

default: ter_test$(EXE_SUFFIX)
ifdef USE_STI2C_WRAPPER
ter_test$(EXE_SUFFIX): $(LINUX_OBJS) $(IMPORT_LIBS) $(STI2C_IOCTL_LIB) $(STTUNER_IOCTL_LIB)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)
else
ter_test$(EXE_SUFFIX): $(LINUX_OBJS) $(IMPORT_LIBS)  $(STTUNER_IOCTL_LIB)
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


ifeq "$(DVD_STTUNER_MINIDRIVER)" "YES"
   CFLAGS := $(CFLAGS) -DSTTUNER_MINIDRIVER
   STTUNER_MINIDRIVER := 1
endif


ifeq "$(STTBX_NO_UART)" "YES"
 CFLAGS := $(CFLAGS) -DSTTBX_NO_UART
endif

ST20_IMPORTS	= stos stcommon stboot stpio stuart sttbx sti2c stevt sttuner
ST40_IMPORTS = stos stcommon stboot stpio sttbx sti2c stevt sttuner
ST200_IMPORTS = stos stcommon stboot stpio sttbx sti2c stevt sttuner

ifndef STTBX_NO_UART
	ST40_IMPORTS += stuart
	ST200_IMPORTS += stuart
endif

$(ARCHITECTURE)_TARGETS := ter_test$(EXE_SUFFIX)

#check whether 370/372 enabled or not
ifeq "$(TUNER_370VSB)" "YES"
TUNER_37X_ENABLED :=1
endif
ifeq "$(TUNER_372)" "YES"
TUNER_37X_ENABLED :=1
endif

# begin environment options ----------------------

# STV0360/STV0361/SV0362 I2C address for the demodulator (default is 0x38)
ifndef DEMOD_I2C
ifndef TUNER_37X_ENABLED
 DEMOD_I2C_0 = 0x38
 DEMOD_I2C_1 = 0x3C
endif

ifdef TUNER_37X_ENABLED
 DEMOD_I2C_0 = 0x18
 DEMOD_I2C_1 = 0x1C
endif
endif

# I2C address for the tuner module if using STV0360/STV0361 (default is0xC0)
ifndef TUNER_I2C
ifndef TUNER_37X_ENABLED
 TUNER_I2C = 0xC0
endif
ifdef TUNER_37X_ENABLED
ifeq "$(TUNER_370VSB)" "YES"
TUNER_I2C = 0xC2
endif
ifeq "$(TUNER_372)" "YES"
TUNER_I2C = 0xC2
endif
endif
endif

# I2C baudrate for I2C bus 100k by default
ifndef I2C_BAUDRATE
          I2C_BAUDRATE = 100000
   #      I2C_BAUDRATE = 90000
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

# if TUNER_360 defined then the tuner type tested is set to STV0360
ifeq "$(TUNER_360)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_360
endif

# if TUNER_361 defined then the tuner type tested is set to STV0361
ifeq "$(TUNER_361)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_361
endif

# if TUNER_362 defined then the tuner type tested is set to STV0362
ifeq "$(TUNER_362)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_362
endif

# To make TPS_CELLID_TEST ON
ifeq "$(TPS_CELLID_TEST)" "YES"
 CFLAGS := $(CFLAGS) -DTPS_CELLID_TEST
endif

ifeq "$(TUNER_370VSB)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_370VSB
endif
ifeq "$(TUNER_372)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_372
endif

# Testing with DTT Transmission (Noida setup only)
ifeq "$(TEST_NOIDA_DTT_TRANSMISSION)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_NOIDA_DTT_TRANSMISSION
endif
ifeq "$(TEST_CHANNEL_ZAP)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_CHANNEL_ZAP
endif
ifeq "$(TEST_SET_30MHZ_REG_SET)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_SET_30MHZ_REG_SET
endif
ifeq "$(TEST_HIERARCHY)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_HIERARCHY
endif
ifeq "$(TEST_CONFIGURE_IF_IQMODE)" "YES"
CFLAGS := $(CFLAGS) -DTEST_CONFIGURE_IF_IQMODE
endif
# RFLevel monitoring 
ifeq "$(DVD_STTUNER_RFLEVEL_MONITOR)" "YES"
 CFLAGS += -DSTTUNER_RF_LEVEL_MONITOR
endif 

# Terrestrial Flag Set
ifeq "$(DVD_STTUNER_USE_TER)" "YES"
 CFLAGS += -DSTTUNER_USE_TER
endif

# Terrestrial Flag Set
ifeq "$(DVD_STTUNER_USE_SAT)" "YES"
 CFLAGS += -DSTTUNER_USE_SAT
endif
# Terrestrial Flag Set
ifeq "$(DVD_STTUNER_USE_CAB)" "YES"
 CFLAGS += -DSTTUNER_USE_CAB
endif

ifeq "$(DVD_STTUNER_USE_ATSC_VSB)" "YES"
 CFLAGS += -DSTTUNER_USE_ATSC_VSB
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

# Tuner type default DTT7572
ifndef TEST_TUNER_TYPE
 TEST_TUNER_TYPE = DTT7592
endif


# --------------------------------------------------------------------
# Variant flags
CFLAGS += -DSTTUNER_BASIC -DSTTUNER_BASIC_ADV -DSTTUNER_BASIC_ECHO
# --------------------------------------------------------------------


# end environment options ------------------------


# STi5508 I2C bug means we need SW start/stop version of i2c driver
ifeq ($(DVD_FRONTEND), 5508)
 export DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP
endif

# software START/STOP is also supported on 5518
ifeq ($(DVD_FRONTEND),5518)
# export DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP
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
TESTDEFS = -DTUNER_TABLE_SIZE=100 -DSTTBX_PRINT -DTUNER_I2C=$(TUNER_I2C) \
           -DDEMOD_I2C_0=$(DEMOD_I2C_0) -DDEMOD_I2C_1=$(DEMOD_I2C_1) -DI2C_BAUDRATE=$(I2C_BAUDRATE) \
           -DTUNER_TRACKING=$(TUNER_TRACKING) \
           -DTUNER_TPS_COUNTER=$(TUNER_TPS_COUNTER) \
           -DTEST_TUNER_TYPE=STTUNER_TUNER_$(TEST_TUNER_TYPE) \
           -DNUM_TUNER=$(NUM_TUNER) -DRELOCK_TEST_NUMTIMES=$(RELOCK_TEST_NUMTIMES)
           

ifndef STTUNER_MINIDRIVER
TESTDEFS += -DSTEVT_ENABLED
endif
# Append TESTDEFS
CFLAGS := $(CFLAGS) $(TESTDEFS)

# Objects to build
ST20_OBJS = ter_test.tco
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))


# Default rules
include $(DVD_MAKE)/defrules.mak

ter_test_INIT := board_init

ter_test$(EXE_SUFFIX): $($(ARCHITECTURE)_OBJS) $(IMPORT_LIBS)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)
	@-$(MKDIR) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@-$(CP) *.map objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@-$(CP) *.dbg objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@$(CP) *$(EXE_SUFFIX) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)

#debug:
#	@echo target selected : $(TARGET)
#ifneq "$(wildcard $(ST20ROOT)/bin/st20dev.exe)" ""
#	st20dev -g -l $(DVD_MAKE) -i $(DVD_PLATFORM).cfg -t $(TARGET) ter_test.lku
#else
#	st20run -g -l $(DVD_MAKE) -i $(DVD_PLATFORM).cfg -t $(TARGET) ter_test.lku
#endif
	

#run:
#	@echo target selected : $(TARGET)
#ifeq ($(ARCHITECTURE),ST20)
#	st20run -l $(DVD_MAKE) -i $(DVD_PLATFORM).cfg -t $(TARGET) ter_test.lku
#endif
#ifeq ($(ARCHITECTURE),ST40)
# ifeq ($(filter $(DVD_FRONTEND),7100),$(DVD_FRONTEND))
#	sh4xrun   -c $(DVD_PLATFORM)bypass -t $(TEST_TARGET) -e ter_test.exe
# else
#	sh4xrun   -c $(DVD_PLATFORM) -t $(TEST_TARGET) -e ter_test.exe
# endif
#endif
	

#log test output to file
runlog:
	@echo target selected : $(TARGET)
	$(RM) ter.log
	st20run -log ter.log -l $(DVD_MAKE) -i $(DVD_PLATFORM).cfg -t $(TARGET) ter_test.lku

clean:
	$(RM) *.lib
	$(RM) *.lku
	$(RM) *.exe
	$(RM) *.tco
	$(RM) *.o
	$(RM) *.dbg
	$(RM) *.map
	$(RM) *.log
	-$(FULLRM) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	-$(RMDIR) objs


endif #corresponds to else of -> ifeq "$(DVD_OS)" "LINUX"      Initial line in this makefile

#eof
