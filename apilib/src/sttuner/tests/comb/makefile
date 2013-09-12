# file: sttuner/tests/comb/makefile
# makes: comb_test.lku


DVD_MAKE_VERSION := 2

ifeq "$(DVD_OS)" "LINUX"
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ifdef USE_STI2C_WRAPPER
HEADER_IMPORTS += stos stcommon stevt sttuner 
LINUX_IMPORTS += stos stcommon stevt 
else
HEADER_IMPORTS += stos stcommon stevt  sti2c sttuner 
LINUX_IMPORTS += stos stcommon stevt sti2c
endif


LINUX_TARGETS := comb_test$(EXE_SUFFIX)

# --------------------------------------------------------------------
# Variant flags
CFLAGS += -DSTTUNER_BASIC -DSTTUNER_BASIC_ADV -DSTTUNER_BASIC_ECHO

# --------------------------------------------------------------------
ifeq "$(DVD_STTUNER_MINIDRIVER)" "YES"
   CFLAGS := $(CFLAGS) -DSTTUNER_MINIDRIVER
   STTUNER_MINIDRIVER := 1
endif

#Flag for use with LINUX STI2C kernel or our wrapper
#ifeq "$(USE_STI2C_WRAPPER)" "YES"
# CFLAGS := $(CFLAGS) -DUSE_STI2C_WRAPPER
#endif
# begin environment options ----------------------


# STV0299/STV0399 I2C address for the demodulator (default is 0xD0)
ifndef SAT_399_DEMOD_I2C
 SAT_399_DEMOD_I2C = 0xD2
endif

ifndef SAT_299_DEMOD_I2C
 SAT_299_DEMOD_I2C = 0xD0
endif

ifndef TER_361_DEMOD_I2C
 TER_361_DEMOD_I2C = 0x38
endif

ifndef TER_360_DEMOD_I2C
 TER_360_DEMOD_I2C = 0x38
endif


ifeq "$(TEST_DEBUG_ENABLE)" "YES"
	CFLAGS:= $(CFLAGS) -DDEBUG
	DEBUG=1
endif


ifndef SAT_TUNER_I2C
 SAT_TUNER_I2C = 0xC0
endif

ifndef TER_TUNER_I2C
 TER_TUNER_I2C = 0xC0
endif

# I2C address for the tuner module if using STV0299 (default is 0xC0 for VG1011. Others:0xC8 for MAX2118,0XC2 for DSF8910)
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_MAX2116"
 SAT_TUNER_I2C=0xC8
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6000"
 SAT_TUNER_I2C=0xC0
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_DSF8910"
 SAT_TUNER_I2C=0xC2
endif
ifeq "STTUNER_SAT_TEST_TUNER_TYPE" "STTUNER_TUNER_STB6100"
 SAT_TUNER_I2C=0xC0
endif
ifeq "STTUNER_SAT_TEST_TUNER_TYPE" "STTUNER_TUNER_STB6110"
 SAT_TUNER_I2C=0xC0
endif
ifeq "STTUNER_TER_TEST_TUNER_TYPE" "STTUNER_TUNER_DTT7592"
 TER_TUNER_I2C=0xC0
endif
ifeq "STTUNER_TER_TEST_TUNER_TYPE" "STTUNER_TUNER_DTT7600"
 TER_TUNER_I2C=0xC2
endif



ifeq "$(DEBUG_288)" "YES"
 CFLAGS := $(CFLAGS) -DDEBUG_288
endif

# Satellite Flag Set 
ifeq "$(DVD_STTUNER_USE_SAT)" "YES"
 CFLAGS += -DSTTUNER_USE_SAT
endif 

# Cable Flag Set
ifeq "$(DVD_STTUNER_USE_CAB)" "YES"
 CFLAGS += -DSTTUNER_USE_CAB
endif

ifeq "$(DVD_TUNER)" "STV0297E"
  CFLAGS := $(CFLAGS) -DTUNER_297E
  TEST_DEMOD_TYPE = STV0297E
  DEMOD_I2C = 0x3a
  XTAL_CLOCK = 25000000
  ifndef TEST_TUNER_TYPE
    TEST_TUNER_TYPE = DCT7045
    endif  	
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


ifeq "$(DVD_STTUNER_BLOCK_SYNC_MODE_SELECT)" "YES"
 CFLAGS := $(CFLAGS) -DSTTUNER_BLOCK_SYNC_MODE_SELECT
endif

# if NUM_TUNER > 1 and TUNER_FIRST defined then tuner #2 is tested first,
# else tuner #1 is tested first.
ifeq "$(TUNER_FIRST)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_FIRST
endif

# if TUNER_399 defined then the tuner type tested is set to STV0399
# else it is set to STV0299.
ifeq "$(TUNER_399)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_399
endif

ifeq "$(TUNER_299)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_299
endif

ifeq "$(TUNER_288)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_288
endif

ifeq "$(TUNER_372)" "YES"
CFLAGS := $(CFLAGS) -DTUNER_372
endif


ifeq "$(TUNER_362)" "YES"
CFLAGS := $(CFLAGS) -DTUNER_362
endif

ifeq "$(TUNER_289)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_289
endif

ifeq "$(TUNER_899)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_899
endif

ifeq "$(TUNER_ADV1)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_ADV1
endif

ifeq "$(LNB_21)" "YES"
 CFLAGS := $(CFLAGS) -DLNB_21
endif
ifeq "$(LNBH_21)" "YES"
 CFLAGS := $(CFLAGS) -DLNBH_21
endif

ifeq "$(STC1_LEGACY)" "YES"
 CFLAGS := $(CFLAGS) -DSTC1_LEGACY
endif

ifeq "$(SAT_TEST_SCR)" "YES"
 CFLAGS := $(CFLAGS) -DSAT_TEST_SCR
endif

ifeq "$(SAT_TEST_DISHPOSITIONING)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_DISHPOSITIONING
endif

ifeq "$(TEST_DISEQC_5100)" "YES"
  CFLAGS := $(CFLAGS) -DTEST_DISEQC_5100
endif

ifeq "$(TEST_DISEQC_5301)" "YES"
  CFLAGS := $(CFLAGS) -DTEST_DISEQC_5301
endif

#Special setup for For Mediaref 5514 with STV0299 on a STEM card
ifeq "$(MREF_STEM_299)" "YES"
 CFLAGS := $(CFLAGS) -DMREF_STEM_299
endif

# Dual tuner STEM card
ifeq "$(DUAL_STEM_299)" "YES"
 CFLAGS := $(CFLAGS) -DDUAL_STEM_299
endif

# Testing with C-Band (Noida setup only)
ifeq "$(C_BAND_TESTING)" "YES"
 CFLAGS := $(CFLAGS) -DC_BAND_TESTING
endif

# number of times to do remove/reapply signal feed (antenna disconnection test)
ifndef RELOCK_TEST_NUMTIMES
 RELOCK_TEST_NUMTIMES = 2
endif

# Tuner tracking test number of iterations (default 60 times)
ifndef TUNER_TRACKING
 TUNER_TRACKING = 20
endif

# Tuner type default VG1011. Others:MAX2116,DSF8910,STB6000
ifndef TEST_TUNER_TYPE
 TEST_TUNER_TYPE = STTUNER_TUNER_VG1011
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_S68G21"
   TEST_TUNER_TYPE = STTUNER_TUNER_S68G21
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_VG1011"
   TEST_TUNER_TYPE = STTUNER_TUNER_VG1011
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_TUA6100"
 TEST_TUNER_TYPE = STTUNER_TUNER_TUA6100
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_EVALMAX"
  TEST_TUNER_TYPE = STTUNER_TUNER_EVALMAX
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_VG0011"
   TEST_TUNER_TYPE = STTUNER_TUNER_VG0011
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_HZ1184"
 TEST_TUNER_TYPE = STTUNER_TUNER_HZ1184
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_MAX2116"
 TEST_TUNER_TYPE = STTUNER_TUNER_MAX2116
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6100"
 TEST_TUNER_TYPE = STTUNER_TUNER_STB6100
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6110"
 TEST_TUNER_TYPE = STTUNER_TUNER_STB6110
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6000"
 TEST_TUNER_TYPE = STTUNER_TUNER_STB6000
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_DSF8910"
 TEST_TUNER_TYPE = STTUNER_TUNER_DSF8910
endif

# Stacked LNB
ifeq "$(USA_STACK_LNB)" "YES"
 CFLAGS := $(CFLAGS) -DUSA_STACK_LNB 
endif

# Test definitions required for build of test harness
TESTDEFS = -DTUNER_TABLE_SIZE=100 -DSTTBX_PRINT -DTUNER_I2C=$(TUNER_I2C) \
           -DDEMOD_I2C=$(DEMOD_I2C) -DI2C_BAUDRATE=$(I2C_BAUDRATE) \
           -DTUNER_TRACKING=$(TUNER_TRACKING) \
           -DTEST_TUNER_TYPE=$(TEST_TUNER_TYPE) \
           -DRELOCK_TEST_NUMTIMES=$(RELOCK_TEST_NUMTIMES) 

ifndef STTUNER_MINIDRIVER
TESTDEFS += -DSTEVT_ENABLED
endif

# Append TESTDEFS
CFLAGS := $(CFLAGS) $(TESTDEFS)

LINUX_OBJS = comb_test$(OBJ_SUFFIX)

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

default: comb_test$(EXE_SUFFIX)
ifdef USE_STI2C_WRAPPER
comb_test$(EXE_SUFFIX): $(LINUX_OBJS) $(IMPORT_LIBS) $(STI2C_IOCTL_LIB) $(STTUNER_IOCTL_LIB)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)
else
comb_test$(EXE_SUFFIX): $(LINUX_OBJS) $(IMPORT_LIBS) $(STTUNER_IOCTL_LIB)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)

endif
#log test output to file
runlog: 
	$(MAKE) run DVD_RUNARGS="-log sat.log"

clean:
	$(RM) *.lib
	$(RM) *.a
	$(RM) *.lku
	$(RM) *.exe
	$(RM) *.tco
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


# file: sttuner/tests/sat/makefile
# makes: comb_test.lku
# history:(d/m/y)
#          24/05/05 - combined makefile for SAT+TER
#

DVD_MAKE_VERSION := 2

# Pick up standard variables
include $(DVD_MAKE)/generic.mak

CFLAGS += -DSTTUNER_BASIC -DSTTUNER_BASIC_ADV -DSTTUNER_BASIC_ECHO

ifeq "$(STTBX_NO_UART)" "YES"
 CFLAGS := $(CFLAGS) -DSTTBX_NO_UART
endif
ifdef DISABLE_TOOLBOX
 CFLAGS += -DDISABLE_TOOLBOX
endif

# Testing with DTT Transmission (Noida setup only)
ifeq "$(TEST_NOIDA_DTT_TRANSMISSION)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_NOIDA_DTT_TRANSMISSION
endif

ifndef DISABLE_TOOLBOX
IMPORTS	= stos stcommon stboot stpio sttbx sti2c stevt sttuner
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

ST20_TARGETS = comb_test.lku

ifeq "$(DVD_STTUNER_MINIDRIVER)" "YES"
   CFLAGS := $(CFLAGS) -DSTTUNER_MINIDRIVER
   STTUNER_MINIDRIVER := 1
endif

ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST200_TARGETS = $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))

# begin environment options ----------------------

# STV0299/STV0399 I2C address for the demodulator (default is 0xD0)
ifndef SAT_399_DEMOD_I2C
 SAT_399_DEMOD_I2C = 0xD2
endif

ifndef SAT_299_DEMOD_I2C
 SAT_299_DEMOD_I2C = 0xD0
endif

ifndef TER_361_DEMOD_I2C
 TER_361_DEMOD_I2C = 0x38
endif

ifndef TER_360_DEMOD_I2C
 TER_360_DEMOD_I2C = 0x38
endif

# I2C address for the tuner module if using STV0299 (default is 0xC0 for VG1011. Others:0xC8 for MAX2118,0XC2 for DSF8910)
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_MAX2116"
 SAT_TUNER_I2C=0xC8
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6000"
 SAT_TUNER_I2C=0xC0
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_DSF8910"
 SAT_TUNER_I2C=0xC2
endif
ifeq "STTUNER_SAT_TEST_TUNER_TYPE" "STTUNER_TUNER_STB6100"
 SAT_TUNER_I2C=0xC0
endif
ifeq "STTUNER_SAT_TEST_TUNER_TYPE" "STTUNER_TUNER_STB6110"
 SAT_TUNER_I2C=0xC0
endif
ifeq "STTUNER_TER_TEST_TUNER_TYPE" "STTUNER_TUNER_DTT7592"
 TER_TUNER_I2C=0xC0
endif
ifeq "STTUNER_TER_TEST_TUNER_TYPE" "STTUNER_TUNER_DTT7600"
 TER_TUNER_I2C=0xC2
endif


ifndef SAT_TUNER_I2C
 SAT_TUNER_I2C = 0xC0
endif

ifndef TER_TUNER_I2C
 TER_TUNER_I2C = 0xC0
endif

# I2C address for the tuner module if using STV0360/STV0361 (default is0xC0)
ifndef TUNER_I2C
ifndef TUNER_37X_ENABLED
 TER_TUNER_I2C = 0xC0
endif
ifdef TUNER_37X_ENABLED
ifeq "$(TUNER_370VSB)" "YES"
 TER_TUNER_I2C = 0xC2
endif
ifeq "$(TUNER_372)" "YES"
 TER_TUNER_I2C = 0xC2
endif
endif
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


ifeq "$(DVD_STTUNER_BLOCK_SYNC_MODE_SELECT)" "YES"
 CFLAGS := $(CFLAGS) -DSTTUNER_BLOCK_SYNC_MODE_SELECT
endif

# if NUM_TUNER > 1 and TUNER_FIRST defined then tuner #2 is tested first,
# else tuner #1 is tested first.
#ifeq "$(TUNER_FIRST)" "YES"
 #CFLAGS := $(CFLAGS) -DTUNER_FIRST
#endif

# if TUNER_299_360 defined then the tuner type tested is set to STV0360
ifeq "$(TUNER_299_360)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_299_360
endif

ifeq "$(TUNER_372)" "YES"
CFLAGS := $(CFLAGS) -DTUNER_372
endif

ifeq "$(TUNER_362)" "YES"
CFLAGS := $(CFLAGS) -DTUNER_362
endif

ifeq "$(TUNER_288)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_288
endif
# if TUNER_288_372 defined then the tuner type tested is set to STV0360
ifeq "$(TUNER_288_372)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_288_372
endif

# if TUNER_299_361 defined then the tuner type tested is set to STV0361
ifeq "$(TUNER_299_361)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_299_361
endif

ifeq "$(TUNER_299_370VSB)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_299_370VSB
endif

# if TUNER_399_360 defined then the tuner type tested is set to STV0360
ifeq "$(TUNER_399_360)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_399_360
endif

# if TUNER_399_361 defined then the tuner type tested is set to STV0361
ifeq "$(TUNER_399_361)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_399_361
endif

ifeq "$(TUNER_399_370VSB)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_399_370VSB
endif

# if TUNER_899_360 defined then the tuner type tested is set to STV0360
ifeq "$(TUNER_899_360)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_899_360
endif

ifeq "$(TUNER_899)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_899
endif

# if TUNER_899_361 defined then the tuner type tested is set to STV0361
ifeq "$(TUNER_899_361)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_899_361
endif

ifeq "$(TUNER_899_370VSB)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_899_370VSB
endif

ifeq "$(LNB_21)" "YES"
 CFLAGS := $(CFLAGS) -DLNB_21
endif
ifeq "$(LNBH_21)" "YES"
 CFLAGS := $(CFLAGS) -DLNBH_21
endif

ifeq "$(SAT_TEST_SCR)" "YES"
 CFLAGS := $(CFLAGS) -DSAT_TEST_SCR
endif

ifeq "$(SAT_TEST_DISHPOSITIONING)" "YES"
 CFLAGS := $(CFLAGS) -DTEST_DISHPOSITIONING
endif

ifeq "$(TEST_DISEQC_5100)" "YES"
  CFLAGS := $(CFLAGS) -DTEST_DISEQC_5100
endif

#Special setup for For Mediaref 5514 with STV0299 on a STEM card
ifeq "$(MREF_STEM_299)" "YES"
 CFLAGS := $(CFLAGS) -DMREF_STEM_299
endif

# Dual tuner STEM card
ifeq "$(DUAL_STEM_299)" "YES"
 CFLAGS := $(CFLAGS) -DDUAL_STEM_299
endif

# Testing with C-Band (Noida setup only)
ifeq "$(C_BAND_TESTING)" "YES"
 CFLAGS := $(CFLAGS) -DC_BAND_TESTING
endif

# number of times to do remove/reapply signal feed (antenna disconnection test)
ifndef RELOCK_TEST_NUMTIMES
 RELOCK_TEST_NUMTIMES = 2
endif

# Tuner tracking test number of iterations (default 60 times)
ifndef TUNER_TRACKING
 TUNER_TRACKING = 20
endif

# Tuner type default VG1011. Others:MAX2116,DSF8910,STB6000
ifndef TEST_SAT_TUNER_TYPE
 TEST_SAT_TUNER_TYPE = STTUNER_TUNER_VG1011
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_S68G21"
   TEST_SAT_TUNER_TYPE = STTUNER_TUNER_S68G21
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_VG1011"
   TEST_SAT_TUNER_TYPE = STTUNER_TUNER_VG1011
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_TUA6100"
 TEST_SAT_TUNER_TYPE = STTUNER_TUNER_TUA6100
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_EVALMAX"
  TEST_SAT_TUNER_TYPE = STTUNER_TUNER_EVALMAX
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_VG0011"
   TEST_SAT_TUNER_TYPE = STTUNER_TUNER_VG0011
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_HZ1184"
 TEST_SAT_TUNER_TYPE = STTUNER_TUNER_HZ1184
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_MAX2116"
 TEST_SAT_TUNER_TYPE = STTUNER_TUNER_MAX2116
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6100"
 TEST_SAT_TUNER_TYPE = STTUNER_TUNER_STB6100
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6110"
 TEST_SAT_TUNER_TYPE = STTUNER_TUNER_STB6110
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6000"
 TEST_SAT_TUNER_TYPE = STTUNER_TUNER_STB6000
endif
ifeq "$(STTUNER_SAT_TEST_TUNER_TYPE)" "STTUNER_TUNER_DSF8910"
 TEST_SAT_TUNER_TYPE = STTUNER_TUNER_DSF8910
endif

ifeq "$(STTUNER_TER_TEST_TUNER_TYPE)" "STTUNER_TUNER_DTT7592"
 TEST_TER_TUNER_TYPE = STTUNER_TUNER_DTT7592
endif
ifeq "$(STTUNER_TER_TEST_TUNER_TYPE)" "STTUNER_TUNER_DTT7600"
 TEST_TER_TUNER_TYPE = STTUNER_TUNER_DTT7600
endif
ifeq "$(STTUNER_TER_TEST_TUNER_TYPE)" "STTUNER_TUNER_DTT7300X"
 TEST_TER_TUNER_TYPE = STTUNER_TUNER_DTT7300X
endif

ifeq "$(TEST_TUNER_TYPE)" "DTT7300X"
 TEST_TUNER_TYPE = STTUNER_TUNER_DTT7300X
endif

ifeq "$(TEST_TUNER_TYPE)" "DTT7600"
 TEST_TUNER_TYPE = STTUNER_TUNER_DTT7600
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



# end environment options ------------------------


# STi5508 I2C bug means we need SW start/stop version of i2c driver
ifeq ($(DVD_FRONTEND), 5508)
 export DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP
endif

# software START/STOP is also supported on 5518
ifeq ($(DVD_FRONTEND),5518)
 export DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP
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
           -DDEMOD_I2C=$(DEMOD_I2C) -DI2C_BAUDRATE=$(I2C_BAUDRATE) \
           -DTUNER_TRACKING=$(TUNER_TRACKING) \
           -DTEST_SAT_TUNER_TYPE=$(TEST_SAT_TUNER_TYPE) \
           -DTEST_TER_TUNER_TYPE=$(TEST_TER_TUNER_TYPE) \
           -DTEST_TUNER_TYPE=$(TEST_TUNER_TYPE)\
           -DRELOCK_TEST_NUMTIMES=$(RELOCK_TEST_NUMTIMES)\
           -DTER_TUNER_I2C=$(TER_TUNER_I2C)  

ifndef STTUNER_MINIDRIVER
TESTDEFS += -DSTEVT_ENABLED
endif

# Append TESTDEFS
CFLAGS := $(CFLAGS) $(TESTDEFS)

# Objects to build
OBJS = comb_test.tco
ST20_OBJS = $(OBJS)
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(OBJS))
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(OBJS))

# Default rules
include $(DVD_MAKE)/defrules.mak

comb_test_INIT := board_init
#$(ST20_TARGETS): $(OBJS) $(IMPORT_LIBS)
#	$(ECHO) Building $@
#	$(LINK_EXECUTABLE)
#$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS)
#	$(ECHO) Building $@
#	$(LINK_EXECUTABLE)
comb_test$(EXE_SUFFIX): $($(ARCHITECTURE)_OBJS) $(IMPORT_LIBS)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)

#log test output to file
runlog: 
	$(MAKE) run DVD_RUNARGS="-log sat.log"

clean:
	$(RM) *.lib
	$(RM) *.a
	$(RM) *.lku
	$(RM) *.exe
	$(RM) *.tco
	$(RM) *.o
	$(RM) *.dbg
	$(RM) *.map
	$(RM) *.log

# -------------------------------------------------------------------------------------
# depreciated:

#run:
#	@echo target selected : $(TARGET)
#	st20run -l $(DVD_MAKE) -i $(DVD_PLATFORM).cfg -t $(TARGET) comb_test.lku

endif #corresponds to else of -> ifeq "$(DVD_OS)" "LINUX"      Initial line in this makefile

#eof

