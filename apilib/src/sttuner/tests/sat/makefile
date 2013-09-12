# file: sttuner/tests/sat/makefile
# makes: sat_test.lku
# history:(d/m/y)
#          8/8/2001 - Modified from 2.0.3 test makefile (bypassed 3.0.0)
#                     to support testing of STTUNER 3.1.0
#
#         22/8/2001 - Added STV0399 support
#
#         10/9/2001 - change name from makefile to sat.mak GJP
#        27/11/2001 - Allow C++ style comments

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


LINUX_TARGETS := sat_test$(EXE_SUFFIX)

# --------------------------------------------------------------------
# Variant flags
CFLAGS += -DSTTUNER_BASIC -DSTTUNER_BASIC_ADV -DSTTUNER_BASIC_ECHO

# --------------------------------------------------------------------
ifeq "$(DVD_STTUNER_MINIDRIVER)" "YES"
   CFLAGS := $(CFLAGS) -DSTTUNER_MINIDRIVER
   STTUNER_MINIDRIVER := 1
endif

#Flag for use with LINUX STI2C kernel or our wrapper
ifeq "$(USE_STI2C_WRAPPER)" "YES"
 CFLAGS := $(CFLAGS) -DUSE_STI2C_WRAPPER
endif
# begin environment options ----------------------

# STV0299/STV0399 I2C address for the demodulator (default is 0xD0)
ifndef DEMOD_I2C
 DEMOD_I2C = 0xD0
endif

ifeq "$(TEST_DEBUG_ENABLE)" "YES"
	CFLAGS:= $(CFLAGS) -DDEBUG
	DEBUG=1
endif

# I2C address for the tuner module if using STV0299 (default is 0xC0 for VG1011. Others:0xC8 for MAX2118,0XC2 for DSF8910)
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_MAX2116"
 TUNER_I2C=0xC8
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6000"
 TUNER_I2C=0xC0
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6110"
 TUNER_I2C=0xC0
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_DSF8910"
 TUNER_I2C=0xC2
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6100"
 TUNER_I2C=0xC0
endif
ifndef TUNER_I2C
 TUNER_I2C = 0xC0
endif

ifeq "$(TUNER_STC1)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_STC1
endif

ifeq "$(DEBUG_288)" "YES"
 CFLAGS := $(CFLAGS) -DDEBUG_288
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

ifeq "$(DVD_STTUNER_DRV_SAT_LNBH24)" "YES"
CFLAGS += -DSTTUNER_DRV_SAT_LNBH24
endif

ifeq "$(DVD_STTUNER_DRV_SAT_LNBH23)" "YES"
CFLAGS += -DSTTUNER_DRV_SAT_LNBH23
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
ifeq "$(STTUNER_TEST_NUM_TUNER)" "2"
  NUM_TUNER = 2
endif
ifeq "$(STTUNER_TEST_NUM_TUNER)" "1"
 NUM_TUNER = 1
endif
ifndef STTUNER_TEST_NUM_TUNER
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
           -DNUM_TUNER=$(NUM_TUNER) -DRELOCK_TEST_NUMTIMES=$(RELOCK_TEST_NUMTIMES) 

ifndef STTUNER_MINIDRIVER
TESTDEFS += -DSTEVT_ENABLED
endif

# Append TESTDEFS
CFLAGS := $(CFLAGS) $(TESTDEFS)

LINUX_OBJS = sat_test$(OBJ_SUFFIX)

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

default: sat_test$(EXE_SUFFIX)
ifdef USE_STI2C_WRAPPER
sat_test$(EXE_SUFFIX): $(LINUX_OBJS) $(IMPORT_LIBS) $(STI2C_IOCTL_LIB) $(STTUNER_IOCTL_LIB)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)
else
sat_test$(EXE_SUFFIX): $(LINUX_OBJS) $(IMPORT_LIBS) $(STTUNER_IOCTL_LIB)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)

endif
#log test output to file
runlog: 
	$(MAKE) run DVD_RUNARGS="-log sat.log"

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
default: sat_test$(EXE_SUFFIX)

$(ARCHITECTURE)_TARGETS := sat_test$(EXE_SUFFIX)


ifndef DISABLE_TOOLBOX
ST20_IMPORTS	= stos stcommon stboot stpio sttbx sti2c stevt sttuner
ST40_IMPORTS = stos stcommon stboot stpio sttbx sti2c stevt sttuner
ST200_IMPORTS = stos stcommon stboot stpio sttbx sti2c stevt sttuner
 ifndef STTBX_NO_UART
    	ST20_IMPORTS += stuart
        ifneq "$(DVD_OS)" "LINUX"
	ST40_IMPORTS += stuart
        endif
	ST200_IMPORTS += stuart
 endif

else

 ST20_IMPORTS = stos stcommon stboot stpio sti2c stevt sttuner
 ST40_IMPORTS = stos stcommon stboot stpio sti2c stevt sttuner
 ST200_IMPORTS = stos stcommon stboot stpio sti2c stevt sttuner

endif


# --------------------------------------------------------------------
# Variant flags
CFLAGS += -DSTTUNER_BASIC -DSTTUNER_BASIC_ADV -DSTTUNER_BASIC_ECHO

# --------------------------------------------------------------------
ifeq "$(DVD_STTUNER_MINIDRIVER)" "YES"
   CFLAGS := $(CFLAGS) -DSTTUNER_MINIDRIVER
   STTUNER_MINIDRIVER := 1
endif

# begin environment options ----------------------

# STV0299/STV0399 I2C address for the demodulator (default is 0xD0)
ifndef DEMOD_I2C
 DEMOD_I2C = 0xD0
endif

ifeq "$(TEST_DEBUG_ENABLE)" "YES"
	CFLAGS:= $(CFLAGS) -DDEBUG
	DEBUG=1
endif

# I2C address for the tuner module if using STV0299 (default is 0xC0 for VG1011. Others:0xC8 for MAX2118,0XC2 for DSF8910)
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_MAX2116"
 TUNER_I2C=0xC8
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6000"
 TUNER_I2C=0xC0
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6110"
 TUNER_I2C=0xC0
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_DSF8910"
 TUNER_I2C=0xC2
endif
ifeq "$(STTUNER_TEST_TUNER_TYPE)" "STTUNER_TUNER_STB6100"
 TUNER_I2C=0xC0
endif
ifndef TUNER_I2C
 TUNER_I2C = 0xC0
endif

ifeq "$(TUNER_STC1)" "YES"
 CFLAGS := $(CFLAGS) -DTUNER_STC1
endif

ifeq "$(DEBUG_288)" "YES"
 CFLAGS := $(CFLAGS) -DDEBUG_288
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

ifeq "$(DVD_STTUNER_DRV_SAT_LNBH24)" "YES"
CFLAGS += -DSTTUNER_DRV_SAT_LNBH24
endif

ifeq "$(DVD_STTUNER_DRV_SAT_LNBH23)" "YES"
CFLAGS += -DSTTUNER_DRV_SAT_LNBH23
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
ifeq "$(STTUNER_TEST_NUM_TUNER)" "2"
  NUM_TUNER = 2
endif
ifeq "$(STTUNER_TEST_NUM_TUNER)" "1"
 NUM_TUNER = 1
endif
ifndef STTUNER_TEST_NUM_TUNER
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
           -DTEST_TUNER_TYPE=$(TEST_TUNER_TYPE) \
           -DNUM_TUNER=$(NUM_TUNER) -DRELOCK_TEST_NUMTIMES=$(RELOCK_TEST_NUMTIMES) 

ifndef STTUNER_MINIDRIVER
TESTDEFS += -DSTEVT_ENABLED
endif

# Append TESTDEFS
CFLAGS := $(CFLAGS) $(TESTDEFS)


#ST20_OBJS = sat_test.tco
#ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Objects to build (all c files in current directory)
OBJS :=  sat_test$(OBJ_SUFFIX) # $(subst .c,.tco,$(wildcard *.c))

# Default rules
include $(DVD_MAKE)/defrules.mak

#sat_test_INIT := board_init

sat_test$(EXE_SUFFIX): $(OBJS) $(IMPORT_LIBS)
	$(ECHO) Building $@
	$(LINK_EXECUTABLE)
	@-$(MKDIR) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@-$(CP) *.map objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@-$(CP) *.dbg objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@$(CP) *$(EXE_SUFFIX) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)

default: sat_test$(EXE_SUFFIX)

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
	-$(FULLRM) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	-$(RMDIR) objs



endif #corresponds to else of -> ifeq "$(DVD_OS)" "LINUX"      Initial line in this makefile

#eof
