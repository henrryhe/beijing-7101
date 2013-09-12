DVD_MAKE_VERSION := 2

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependent upon. These are the
# requirements common to each device.
ifeq "$(DVD_OS)" "LINUX"
IMPORTS := stos stdenc stvtg stevt \
          stcommon stttx  testtool
else
IMPORTS := stdenc stvtg sttbx stevt stpio stboot \
          stcommon stuart stttx testtool sti2c
endif

# Here we have the device specific imports.
# (Should we migrate from using frontend to platform? Or both, maybe?)

ifeq "$(findstring $(DVD_FRONTEND),5105 7710 5107)" "$(DVD_FRONTEND)"
    IMPORTS := $(IMPORTS) stpti4lt
else 
ifeq "$(DVD_FRONTEND)" "5188"
    IMPORTS := $(IMPORTS) stdemux
else
    IMPORTS := $(IMPORTS) $(DVD_TRANSPORT)
endif
endif

ifeq "$(DVD_FRONTEND)" "5514"
    IMPORTS := $(IMPORTS) stgpdma stcfg sttsmux stpwm stclkrv
endif

ifeq "$(DVD_FRONTEND)" "5528"
    IMPORTS := $(IMPORTS) stvout stgpdma stmerge stcfg stpwm stclkrv
endif

ifeq "$(DVD_FRONTEND)" "5516"
    IMPORTS := $(IMPORTS) stcfg sttsmux stpwm stclkrv
endif

ifeq "$(DVD_FRONTEND)" "5517"
    # fdma required for stavmem.
    IMPORTS := $(IMPORTS) stcfg sttsmux stpwm stclkrv
endif

ifeq "$(DVD_FRONTEND)" "5100"
    IMPORTS := $(IMPORTS) stmerge stvout stfdma stlayer stdisp compo stos
endif

ifeq "$(DVD_FRONTEND)" "5301"
    IMPORTS := $(IMPORTS) stmerge stfdma
endif

ifeq "$(DVD_FRONTEND)" "7710"
    IMPORTS := $(IMPORTS) stmerge stfdma stclkrv stlayer stavmem stos stgxobj compo
endif

ifeq "$(DVD_FRONTEND)" "7100"
    IMPORTS := $(IMPORTS) stmerge stpio stfdma stclkrv stlayer stavmem stos stgxobj stvout
    LINUX_libstavmem.a_NOLINK := 1
    LINUX_libstfdma.a_NOLINK := 1
endif

ifeq "$(DVD_FRONTEND)" "7109"
    IMPORTS := $(IMPORTS) stmerge stpio stfdma stclkrv stlayer stavmem stos stgxobj stvout
    LINUX_libstavmem.a_NOLINK := 1
    LINUX_libstfdma.a_NOLINK := 1
endif

ifeq "$(DVD_FRONTEND)" "5105"
    IMPORTS := $(IMPORTS) compo stgxobj stfdma stvout stavmem stvmix stlayer stdisp stos
endif

ifeq "$(DVD_FRONTEND)" "5107"
    IMPORTS := $(IMPORTS) compo stgxobj stfdma stvout stavmem stvmix stlayer stdisp stos
endif

ifeq "$(DVD_FRONTEND)" "5188"
    IMPORTS := $(IMPORTS) compo stgxobj stfdma stvout stavmem stvmix stlayer stdisp stos
endif

ifeq "$(DVD_BACKEND)" "7020"
    IMPORTS := $(IMPORTS) stintmr stfdma stavmem
endif

# add stvbi, if required
ifndef STTTX_USE_DENC
  IMPORTS := $(IMPORTS) stvbi
endif

ifndef STTTX_NO_TUNER
  IMPORTS := $(IMPORTS) sttuner
endif

# these always get built
ifneq "$(DVD_OS)" "LINUX"
TARGETS = ttxdisk$(EXE_SUFFIX) cachedisk$(EXE_SUFFIX) evtdisk$(EXE_SUFFIX)
endif
#pagedisk$(EXE_SUFFIX)

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

ifeq "$(DVD_FRONTEND)" "5528"
  TARGETS := $(TARGETS) ttxdualdisk$(EXE_SUFFIX) ttxdualstpti$(EXE_SUFFIX)
endif

# Building STPTI target?
ifeq "$(DVD_OS)" "LINUX"
  TARGETS := $(TARGETS) ttxstpti$(EXE_SUFFIX)
else
ifeq "$(findstring STPTI, $(DVD_TRANSPORT_UPPERCASE))" "STPTI"
  TARGETS := $(TARGETS) ttxstpti$(EXE_SUFFIX) cachestpti$(EXE_SUFFIX) evtstpti$(EXE_SUFFIX)
else
  ifeq "$(findstring DEMUX, $(DVD_TRANSPORT_UPPERCASE))" "DEMUX"
  TARGETS := $(TARGETS) ttxstpti$(EXE_SUFFIX) cachestpti$(EXE_SUFFIX) evtstpti$(EXE_SUFFIX)
  else
  TARGETS := $(TARGETS) ttxpti$(EXE_SUFFIX) cachepti$(EXE_SUFFIX) evtpti$(EXE_SUFFIX)
  endif
endif
endif

ifdef STTTX_SUBT_SYNC_ENABLE
TARGETS = ttxstpti$(EXE_SUFFIX)
endif

ifeq "$(DVD_BACKEND)" "7020"
  TARGETS := ttxdisk$(EXE_SUFFIX)
endif

ifeq "$(DVD_TRANSPORT_UPPERCASE)" "STPTI4"
  CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_STPTI
endif

ifdef DISABLE_ICACHE
  CFLAGS := $(CFLAGS) -DDISABLE_ICACHE
endif

ifdef DISABLE_DCACHE
  CFLAGS := $(CFLAGS) -DDISABLE_DCACHE
endif

ifdef STTTX_DEBUG1
  CFLAGS := $(CFLAGS) -DSTTTX_DEBUG1
endif

ifdef STVTG_VSYNC_WITHOUT_VIDEO
  CFLAGS := $(CFLAGS) -DSTVTG_VSYNC_WITHOUT_VIDEO
endif

# I2C address for the demodulator 0xD0 by default
ifndef DEMOD_I2C
  DEMOD_I2C = 0xD0
endif

# I2C address for the tuner module 0xC2 by default
ifndef TUNER_I2C
  TUNER_I2C = 0xC0
endif

# I2C baudrate for I2C bus 100k by default
ifndef I2C_BAUDRATE
  I2C_BAUDRATE = 100000
endif

# Demod type default STV0299
ifndef TEST_DEMOD_TYPE
  TEST_DEMOD_TYPE = STV0299
endif

# Tuner type default VG1011
ifndef TEST_TUNER_TYPE
  TEST_TUNER_TYPE = VG1011
endif

# Default PID values
ifndef STB_PID
  STB_PID=0x2f
endif
ifndef VBI_PID
  VBI_PID=0x29
endif
ifndef PCR_PID
  PCR_PID=0x1ffe
endif
ifndef TTX_VBI_PERIOD
  TTX_VBI_PERIOD=60
endif
ifndef TTX_FREQ
  TTX_FREQ=12165
endif
ifndef TTX_POLARITY
  TTX_POLARITY=STTUNER_PLR_VERTICAL
endif
ifndef TTX_FEC
  TTX_FEC=STTUNER_FEC_3_4
endif
ifndef STTTX_SYNC_OFFSET
  STTTX_SYNC_OFFSET=9000
endif

# Test flags
# 
# PCR_PID
# VBI_PID           PID for VBI teletext (normally 0x2f, CNN)
# STB_PID           PID for STB teletext (normally 0x29, Travel)
# TTX_VBI_PERIOD    Delay (in seconds) to allow before ending VBI test (60s)
TESTDEFS = -DSTB_PID=$(STB_PID) -DVBI_PID=$(VBI_PID) \
           -DTTX_VBI_PERIOD=$(TTX_VBI_PERIOD)\
           -DPCR_PID=$(PCR_PID)

# Only add this big wodge of stuff to the flags if we actually need to.
ifndef STTTX_NO_TUNER
  TESTDEFS := $(TESTDEFS) -DPOLARITY=$(TTX_POLARITY) -DFEC=$(TTX_FEC) \
               -DTUNER_I2C=$(TUNER_I2C) -DDEMOD_I2C=$(DEMOD_I2C) \
               -DFREQ_MHZ=$(TTX_FREQ) \
               -DTEST_DEMOD_TYPE=STTUNER_DEMOD_$(TEST_DEMOD_TYPE) \
               -DTEST_TUNER_TYPE=STTUNER_TUNER_$(TEST_TUNER_TYPE)
endif

# Append options to cflags

# Default to testing from a live stream
ifndef STTTX_PES_MODE
  STTTX_PES_MODE = 0
endif

# Add this to the defines list, if the environment variable's set
ifdef STTTX_USE_DENC
  CFLAGS := $(CFLAGS) -DSTTTX_USE_DENC
endif

ifdef STTTX_PES_FILE
  CFLAGS := $(CFLAGS) -DSTTTX_PES_FILE=\"$(STTTX_PES_FILE)\"
endif
    
ifdef STTTX_NOSTEMBOARD
  CFLAGS := $(CFLAGS) -DSTTTX_NOSTEMBOARD
endif

ifdef DVD_TRANSPORT_UPPERCASE
  CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_$(DVD_TRANSPORT_UPPERCASE)
else
  # else default to PTI
  DVD_TRANSPORT_UPPERCASE := PTI
  CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_PTI
endif

ifdef STTTX_OLD_TUNER
  CFLAGS := $(CFLAGS) -DSTTTX_OLD_TUNER=1
endif

ifdef STTTX_SUBT_SYNC_ENABLE
  CFLAGS := $(CFLAGS) -DSTTTX_SUBT_SYNC_ENABLE -DSTTTX_SYNC_OFFSET=$(STTTX_SYNC_OFFSET)
endif

ifeq "$(DVD_BACKEND)" "7020"
  CFLAGS := $(CFLAGS) -DSTTTX_USE_DENC
endif

# unless instructed otherwise, assume use of a tuner
ifndef STTTX_NO_TUNER
   CFLAGS := $(CFLAGS) -DSTTTX_USE_TUNER
endif

# Update CFLAGS
CFLAGS := $(CFLAGS) $(TESTDEFS) -DSTTTX_PES_MODE=$(STTTX_PES_MODE) -DSTTBX_PRINT

# Include default rules
include $(DVD_MAKE)/defrules.mak

ifeq "$(ARCHITECTURE)" "ST200"	

ttxdualdisk$(OBJ_SUFFIX): stttxtst.c
	@$(ECHO) Compiling stttxtst.c to $@
	$(CC) -DTTXDISK_TEST -DTTXDUAL_DISK_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o ttxdualdisk$(OBJ_SUFFIX)

ttxdualstpti$(OBJ_SUFFIX): stttxtst.c
	@$(ECHO) Compiling stttxtst.c to $@
	$(CC) -DTTXSTPTI_TEST -DTTXDUAL_STPTI_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@
	
ttxdisk$(OBJ_SUFFIX): stttxtst.c
	@$(ECHO) Compiling stttxtst.c to $@
	$(CC) -DTTXDISK_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o ttxdisk$(OBJ_SUFFIX)

cachedisk$(OBJ_SUFFIX): ttxcache.c
	@$(ECHO) Compiling ttxcache.c to $@
	$(CC) -DTTXDISK_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o cachedisk$(OBJ_SUFFIX)

pagedisk$(OBJ_SUFFIX): ttxpage.c
	@$(ECHO) Compiling ttxpage.c to $@
	$(CC) -DTTXDISK_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o pagedisk$(OBJ_SUFFIX)

evtdisk$(OBJ_SUFFIX): evtmon.c
	@$(ECHO) Compiling evtmon.c to $@
	$(CC) -DTTXDISK_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o evtdisk$(OBJ_SUFFIX)

else

ttxdualdisk$(OBJ_SUFFIX): stttxtst.c
	@$(ECHO) Compiling stttxtst.c to $@
	$(CC) -DTTXDISK_TEST -DTTXDUAL_DISK_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o ttxdualdisk$(OBJ_SUFFIX)

ttxdualstpti$(OBJ_SUFFIX): stttxtst.c
	@$(ECHO) Compiling stttxtst.c to $@
	$(CC) -DTTXSTPTI_TEST -DTTXDUAL_STPTI_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@
	
ttxdisk$(OBJ_SUFFIX): stttxtst.c
	@$(ECHO) Compiling stttxtst.c to $@
	$(CC) -DTTXDISK_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o ttxdisk$(OBJ_SUFFIX)

cachedisk$(OBJ_SUFFIX): ttxcache.c
	@$(ECHO) Compiling ttxcache.c to $@
	$(CC) -DTTXDISK_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o cachedisk$(OBJ_SUFFIX)

pagedisk$(OBJ_SUFFIX): ttxpage.c
	@$(ECHO) Compiling ttxpage.c to $@
	$(CC) -DTTXDISK_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o pagedisk$(OBJ_SUFFIX)

evtdisk$(OBJ_SUFFIX): evtmon.c
	@$(ECHO) Compiling evtmon.c to $@
	$(CC) -DTTXDISK_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o evtdisk$(OBJ_SUFFIX)

endif

ttxdualdisk$(EXE_SUFFIX): ttxdualdisk$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

ttxdualstpti$(EXE_SUFFIX): ttxdualstpti$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
	
ttxdisk$(EXE_SUFFIX): ttxdisk$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

cachedisk$(EXE_SUFFIX): cachedisk$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

pagedisk$(EXE_SUFFIX): pagedisk$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

evtdisk$(EXE_SUFFIX): evtdisk$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

# these LKUs vary depending on which transport is being used
#ifeq "$(DVD_TRANSPORT_UPPERCASE)" "DEMUX"
ifeq "$(findstring $(DVD_TRANSPORT_UPPERCASE), DEMUX STPTI STPTI4 )" "$(DVD_TRANSPORT_UPPERCASE)"

tt_ttx$(OBJ_SUFFIX): tt_ttx.c
	$(CC) -DTTXINTERACTIVE_TEST -DTTXSTPTI_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

ttxinter$(OBJ_SUFFIX): stttxtst.c
	$(CC) -DTTXINTERACTIVE_TEST -DTTXSTPTI_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

ifndef STTTX_NO_TUNER
ttxinter$(EXE_SUFFIX): ttxinter$(OBJ_SUFFIX) tt_ttx$(OBJ_SUFFIX) tt_tuner$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
else
ttxinter$(EXE_SUFFIX): ttxinter$(OBJ_SUFFIX) tt_ttx$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(ARCHITECTURE)" "ST200"	

ttxstpti$(OBJ_SUFFIX): stttxtst.c
	@$(ECHO) Compiling stttxtst.c to $@
	$(CC) -DTTXSTPTI_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

ttxcache$(OBJ_SUFFIX): ttxcache.c
	@$(ECHO) Compiling ttxcache.c to $@
	$(CC) -DTTXSTPTI_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

evtmon$(OBJ_SUFFIX): evtmon.c
	@$(ECHO) Compiling evtmon.c to $@
	$(CC) -DTTXSTPTI_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

pagestpti$(OBJ_SUFFIX): ttxpage.c
	@$(ECHO) Compiling ttxpage.c to $@
	$(CC) -DTTXSTPTI_TEST $(CFLAGS) -mcore=st231 $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

else

ttxstpti$(OBJ_SUFFIX): stttxtst.c
	@$(ECHO) Compiling stttxtst.c to $@
	$(CC) -DTTXSTPTI_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

ttxcache$(OBJ_SUFFIX): ttxcache.c
	@$(ECHO) Compiling ttxcache.c to $@
	$(CC) -DTTXSTPTI_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

evtmon$(OBJ_SUFFIX): evtmon.c
	@$(ECHO) Compiling evtmon.c to $@
	$(CC) -DTTXSTPTI_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

pagestpti$(OBJ_SUFFIX): ttxpage.c
	@$(ECHO) Compiling ttxpage.c to $@
	$(CC) -DTTXSTPTI_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

endif

ttxstpti$(EXE_SUFFIX): ttxstpti$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

cachestpti$(EXE_SUFFIX): ttxcache$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

evtstpti$(EXE_SUFFIX): evtmon$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

pagestpti$(EXE_SUFFIX): pagestpti$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

else

ifeq "$(DVD_TRANSPORT_UPPERCASE)" "LINK"
    TESTFLAG := TTXLINK_TEST
else
    TESTFLAG := TTXPTI_TEST
endif

ttxinter$(OBJ_SUFFIX): stttxtst.c
	$(CC) -DTTXINTERACTIVE_TEST -D$(TESTFLAG) $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

ifndef STTTX_NO_TUNER
ttxinter$(EXE_SUFFIX): ttxinter$(OBJ_SUFFIX) tt_ttx$(OBJ_SUFFIX) tt_tuner$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
else
ttxinter$(EXE_SUFFIX): ttxinter$(OBJ_SUFFIX) tt_ttx$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif
 
ttxpti$(OBJ_SUFFIX): stttxtst.c
	@$(ECHO) Compiling stttxtst.c to $@
	$(CC) -D$(TESTFLAG) $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

ttxpti$(EXE_SUFFIX): ttxpti$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

ttxcache$(OBJ_SUFFIX): ttxcache.c
	@$(ECHO) Compiling ttxcache.c to $@
	$(CC) -D$(TESTFLAG) $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

cachepti$(EXE_SUFFIX): ttxcache$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

evtmon$(OBJ_SUFFIX): evtmon.c
	@$(ECHO) Compiling evtmon.c to $@
	$(CC) -D$(TESTFLAG) $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

evtpti$(EXE_SUFFIX): evtmon$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

pagepti$(OBJ_SUFFIX): ttxpage.c
	@$(ECHO) Compiling ttxpage.c to $@
	$(CC) -DTTXPTI_TEST $(CFLAGS) $(INCLUDES) $(BASE_INCLUDES) -c $^ -o $@

pagepti$(EXE_SUFFIX): pagepti$(OBJ_SUFFIX) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

endif

clean:
	@$(ECHO) Cleaning $(TARGETS)
	$(RM) *$(OBJ_SUFFIX)
	$(RM) *.dbg
	$(RM) $(TARGETS)

	
