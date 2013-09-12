#  file: sttuner/makefile

#	      3/05/2006 - Added Tuner Specific Build Options
#        20/11/2006 - Added STV0297E support.


#for forcing use of linux.mak for linux
ifeq "$(DVD_OS)" "LINUX"
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
else
include $(DVD_MAKE)/builddir.mak
endif
else #corresponds to -> ifeq "$(DVD_OS)" "LINUX"      Corresponding endif just before EOF



# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
ST20_IMPORTS = stos stcommon stboot stuart stpio sti2c sttbx stevt
ST20_EXPORTS = sttuner.h sttuner.lib
# Build targets
ST20_TARGETS = sttuner.lib

ST40_IMPORTS := stos stcommon stboot stpio sti2c sttbx stevt
ST200_IMPORTS = stos stcommon stboot stpio sti2c sttbx stevt

ifndef STTBX_NO_UART
	ST40_IMPORTS += stuart
	ST200_IMPORTS += stuart
endif

ST40_EXPORTS := sttuner.h $(LIB_PREFIX)sttuner$(LIB_SUFFIX)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_EXPORTS = sttuner.h $(LIB_PREFIX)sttuner$(LIB_SUFFIX)
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

# --------------------------------------------------------------------
# Variant flags
CFLAGS += -DSTTUNER_BASIC -DSTTUNER_BASIC_ADV -DSTTUNER_BASIC_ECHO
# --------------------------------------------------------------------

ifeq "$(DVD_STTUNER_MINIDRIVER)" "YES"
   CFLAGS := $(CFLAGS) -DSTTUNER_MINIDRIVER
   STTUNER_MINIDRIVER := 1
endif
ifeq "$(DVD_STTUNER_DRV_SAT_5188)" "YES"
  STTUNER_DRV_SAT_5188 := 1
  CFLAGS += -DSTTUNER_DRV_SAT_5188
endif


# Option to be set when there is no external tuner

 ifeq "$(DVD_STTUNER_USE_SHARED)" "YES"
  STTUNER_USE_SHARED := 1
  ifeq "$(DVD_STTUNER_DRV_SHARED_NULL)" "YES"
   STTUNER_DRV_SHARED_NULL := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_SHARED_EXTTUNERS)" "YES"
   STTUNER_DRV_SHARED_EXTTUNERS := 1
  endif
 endif
 
 
 ifeq "$(DVD_STTUNER_TUNER_DTT7600)" "YES"
  STTUNER_USE_SHARED := 1
  STTUNER_DRV_SHARED_EXTTUNERS := 1
  STTUNER_DRV_SHARED_TUN_DTT7600:=1
  CFLAGS += -DSTTUNER_DRV_SHARED_TUN_DTT7600
  endif
 
 ifeq "$(DVD_STTUNER_TUNER_MT2131)" "YES"
  STTUNER_USE_SHARED := 1
  STTUNER_DRV_SHARED_EXTTUNERS := 1
  STTUNER_DRV_SHARED_TUN_MT2131:=1
  CFLAGS += -DSTTUNER_DRV_SHARED_TUN_MT2131
  endif
 ifeq "$(DVD_STTUNER_TUNER_DTT768XX)" "YES"
  STTUNER_USE_SHARED := 1
  STTUNER_DRV_SHARED_EXTTUNERS := 1
  STTUNER_DRV_SHARED_TUN_DTT768XX:=1
  CFLAGS += -DSTTUNER_DRV_SHARED_TUN_DTT768XX
  endif

 
#---------------------------------------------------------------------------

# Options to be set for Satellite in driver and application 
# depending on Device selection like STV0399(with Shared Null tuner)
# or STV0299(with external tuner)
# or STEM (with external tuner) 

 ifeq "$(DVD_STTUNER_USE_SAT)" "YES"
  STTUNER_USE_SAT := 1
  ifeq "$(DVD_STTUNER_DRV_SAT_NULL)" "YES"
   STTUNER_DRV_SAT_NULL := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_SAT_STV0299)" "YES"
   STTUNER_DRV_SAT_STV0299 := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_SAT_STX0288)" "YES"
   STTUNER_DRV_SAT_STX0288 := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_SAT_STB0899)" "YES"
   STTUNER_DRV_SAT_STB0899 := 1
  endif
    
  ifeq "$(DVD_STTUNER_DRV_SAT_STV0399)" "YES"
   STTUNER_DRV_SAT_STV0399E := 1
   STTUNER_DRV_SAT_STV0399 := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_SAT_STEM)" "YES"
   STTUNER_DRV_SAT_STEM := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_SAT_DUAL_STEM_299)" "YES"
   STTUNER_DRV_SAT_DUAL_STEM_299 := 1
  endif
  
 endif
#---------------------------------------------------------------------------------

# Options to be set for Cable in driver and application 
# depending on Device selection like STV0297 , STV0297J or STV0297E


 ifeq "$(DVD_STTUNER_USE_CAB)" "YES"
  STTUNER_USE_CAB := 1
  ifeq "$(DVD_STTUNER_DRV_CAB_STV0297)" "YES"
   STTUNER_DRV_CAB_STV0297 := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_CAB_STV0297E)" "YES"
   STTUNER_DRV_CAB_STV0297E := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_CAB_STV0297J)" "YES"
   STTUNER_DRV_CAB_STV0297J := 1
   ifeq "$(DVD_STTUNER_DRV_CAB_J83B)" "YES"
    STTUNER_DRV_CAB_J83B := 1
   else
    ifeq "$(DVD_STTUNER_DRV_CAB_J83C)" "YES"
     STTUNER_DRV_CAB_J83C := 1
    else
     STTUNER_DRV_CAB_J83A := 1
    endif
   endif
   ifeq "$(DVD_STTUNER_DRV_CAB_DOCSIS_FE)" "YES"
    STTUNER_DRV_CAB_DOCSIS_FE := 1
   endif
   ifeq "$(DVD_STTUNER_DRV_CAB_CBR0)" "YES"
    STTUNER_DRV_CAB_CBR0 := 1
   endif
   ifeq "$(DVD_STTUNER_DRV_CAB_CBR1200)" "YES"
    STTUNER_DRV_CAB_CBR1200 := 1
   endif
  endif
  ifeq "$(DVD_STTUNER_DRV_CAB_STB0370QAM)" "YES"
   STTUNER_DRV_CAB_STB0370QAM:= 1 
   ifeq "$(DVD_STTUNER_DRV_CAB_J83B)" "YES"
    STTUNER_DRV_CAB_J83B := 1
   endif  
  endif
  ifeq "$(DVD_STTUNER_DRV_CAB_STV0371)" "YES"
   STTUNER_DRV_CAB_STB0370QAM:= 1   
  endif
  ifeq "$(DVD_STTUNER_DRV_CAB_EXTTUNERS)" "YES"
   STTUNER_DRV_CAB_EXTTUNERS := 1
  endif
 endif
#-----------------------------------------------------------------------------

# Options to be set for Terrestrial in driver and application 
# depending on Device selection like STV0360 or STV0361 

 ifeq "$(DVD_STTUNER_USE_TER)" "YES"
  STTUNER_USE_TER := 1
  ifeq "$(DVD_STTUNER_DRV_TER_STV0360)" "YES"
   STTUNER_DRV_TER_STV0360 := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_TER_STV0361)" "YES"
   STTUNER_DRV_TER_STV0361 := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_TER_STV0362)" "YES"
   STTUNER_DRV_TER_STV0362 := 1
  endif  
  
 endif
 

# Options to be set for Terrestrial(ATSC , VSB driver) in driver and application 
# depending on Device selection like STB0370 

 ifeq "$(DVD_STTUNER_USE_ATSC_VSB)" "YES"
  STTUNER_USE_ATSC_VSB := 1  
  ifeq "$(DVD_STTUNER_DRV_TER_STB0370VSB)" "YES"
   STTUNER_DRV_TER_STB0370VSB := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_TER_STV0371)" "YES"
   STTUNER_DRV_TER_STB0370VSB := 1
  endif
  ifeq "$(DVD_STTUNER_DRV_TER_STV0372)" "YES"
   STTUNER_DRV_TER_STV0372 := 1
  endif
 endif

# --------------------------------------------------------------------

ifdef STTUNER_USE_TER
      STTUNER_USE_TER_PLUS_ATSC_VSB := 1
endif
ifdef STTUNER_USE_ATSC_VSB
	STTUNER_USE_TER_PLUS_ATSC_VSB := 1
endif

#--------Satellite specific Additional  Build options------------------------

ifeq "$(SAT_TEST_SCR)" "YES"
 CFLAGS := $(CFLAGS) -DSAT_TEST_SCR
endif

ifeq "$(DVD_STTUNER_DRV_SAT_F0299)" "YES"
   STTUNER_DRV_SAT_F0299 := 1
endif
  
ifeq "$(DVD_STTUNER_DRV_SAT_LNB21)" "YES"
 STTUNER_DRV_SAT_LNB21:=1
endif

ifeq "$(DVD_STTUNER_DRV_SAT_LNBH21)" "YES"
 STTUNER_DRV_SAT_LNBH21:=1
endif


ifeq "$(DVD_STTUNER_DRV_SAT_LNBH24)" "YES"
 STTUNER_DRV_SAT_LNBH24:=1
endif

ifeq "$(DVD_STTUNER_DRV_SAT_LNBH23)" "YES"
 STTUNER_DRV_SAT_LNBH23:=1 
endif

ifeq "$(DVD_STTUNER_LNB_POWER_THRU_BACKENDGPIO)" "YES"
 STTUNER_LNB_POWER_THRU_BACKENDGPIO := 1
 CFLAGS += -DSTTUNER_LNB_POWER_THRU_BACKENDGPIO
endif

ifeq "$(DVD_STTUNER_LNB_POWER_THRU_DEMODIO)" "YES"
 STTUNER_LNB_POWER_THRU_DEMODIO := 1
 CFLAGS += -DSTTUNER_LNB_POWER_THRU_DEMODIO
endif

ifeq "$(DVD_STTUNER_LNB_TONE_THRU_DEMOD_DISEQC_PIN)" "YES"
 STTUNER_LNB_TONE_THRU_DEMOD_DISEQC_PIN := 1
 CFLAGS += -DSTTUNER_LNB_TONE_THRU_DEMOD_DISEQC_PIN
endif

ifeq "$(DVD_STTUNER_DRV_SAT_SCR)" "YES"
  CFLAGS += -DSTTUNER_DRV_SAT_SCR
 STTUNER_DRV_SAT_SCR:=1
 endif

 ifeq "$(DVD_STTUNER_DRV_SAT_SCR_LOOPTHROUGH)" "YES"
  CFLAGS += -DSTTUNER_DRV_SAT_SCR_LOOPTHROUGH
 STTUNER_DRV_SAT_SCR_LOOPTHROUGH:=1
 endif
 
 ifeq "$(DVD_STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND)" "YES"
 CFLAGS += -DSTTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
 STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND:=1
endif
 
ifeq "$(DVD_STTUNER_BLOCK_SYNC_MODE_SELECT)" "YES"
 CFLAGS += -DSTTUNER_BLOCK_SYNC_MODE_SELECT
 STTUNER_BLOCK_SYNC_MODE_SELECT:=1
endif

ifeq "$(DVD_STTUNER_DRV_SAT_DISEQC2_SWDECODE_VIA_PIO)" "YES"
 CFLAGS += -DSTTUNER_DISEQC2_SWDECODE_VIA_PIO
endif

ifeq "$(DVD_STTUNER_DRV_SET_HIGH_I2C_BAUDRATE)" "YES"
 CFLAGS += -DSTTUNER_DRV_SET_HIGH_I2C_BAUDRATE
 STTUNER_DRV_SET_HIGH_I2C_BAUDRATE:=1
endif

# Select desired config for IQ wiring depending upon Tuner & Demod IQ connections.
ifdef DVD_STTUNER_IQ_WIRING
 ifeq "$(DVD_STTUNER_IQ_WIRING)" "NORMAL"
  CFLAGS += -DSTTUNER_IQ_WIRING=1
 endif
 ifeq "$(DVD_STTUNER_IQ_WIRING)" "INVERTED"
  CFLAGS += -DSTTUNER_IQ_WIRING=-1
 endif
endif





#--------Terrestrial Specific Additional Build Options--------------

ifeq "$(DVD_STTUNER_RFLEVEL_MONITOR)" "YES"
 CFLAGS += -DSTTUNER_RFLEVEL_MONITOR
endif

ifeq "$(DVD_STTUNER_ATCBIT_TUNER_ENABLE)" "YES"
 CFLAGS += -DSTTUNER_ATCBIT_TUNER_ENABLE
endif


#---------for SATELLITE
ifdef STTUNER_USE_SAT

ifeq "$(DVD_STTUNER_TUNER_VG1011)" "YES"
        STTUNER_DRV_SAT_TUN_VG1011:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_VG1011
endif
ifeq "$(DVD_STTUNER_TUNER_TUA6100)" "YES"
	STTUNER_DRV_SAT_TUN_TUA6100:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_TUA6100
endif
ifeq "$(DVD_STTUNER_TUNER_EVALMAX)" "YES"
	STTUNER_DRV_SAT_TUN_EVALMAX:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_EVALMAX
endif
ifeq "$(DVD_STTUNER_TUNER_S68G21)" "YES"
	STTUNER_DRV_SAT_TUN_S68G21:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_S68G21
endif
ifeq "$(DVD_STTUNER_TUNER_VG0011)" "YES"
	STTUNER_DRV_SAT_TUN_VG0011:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_VG0011
endif
ifeq "$(DVD_STTUNER_TUNER_HZ1184)" "YES"
	STTUNER_DRV_SAT_TUN_HZ1184:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_HZ1184
endif
ifeq "$(DVD_STTUNER_TUNER_MAX2116)" "YES"
	STTUNER_DRV_SAT_TUN_MAX2116:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_MAX2116
endif
ifeq "$(DVD_STTUNER_TUNER_MAX2118)" "YES"
	STTUNER_DRV_SAT_TUN_MAX2118:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_MAX2118
endif
ifeq "$(DVD_STTUNER_TUNER_DSF8910)" "YES"
	STTUNER_DRV_SAT_TUN_DSF8910:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_DSF8910
endif
ifeq "$(DVD_STTUNER_TUNER_STB6000)" "YES"
	STTUNER_DRV_SAT_TUN_STB6000:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_STB6000
endif
ifeq "$(DVD_STTUNER_TUNER_STB6110)" "YES"
	STTUNER_DRV_SAT_TUN_STB6110:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_STB6110
endif

ifeq "$(DVD_STTUNER_TUNER_IX2476)" "YES"
	STTUNER_DRV_SAT_TUN_IX2476:=1
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_IX2476
endif

ifeq "$(DVD_STTUNER_TUNER_STB6100)" "YES"
	STTUNER_DRV_SAT_TUN_STB6100:=1
CFLAGS += -DSTTUNER_DRV_SAT_TUN_STB6100
endif

endif
#--------for TER
ifdef STTUNER_USE_TER 

ifeq "$(DVD_STTUNER_TUNER_DTT7572)" "YES"
	STTUNER_DRV_TER_TUN_DTT7572:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_DTT7572	
endif
ifeq "$(DVD_STTUNER_TUNER_DTT7578)" "YES"
	STTUNER_DRV_TER_TUN_DTT7578:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_DTT7578
endif
ifeq "$(DVD_STTUNER_TUNER_DTT7592)" "YES"
	STTUNER_DRV_TER_TUN_DTT7592:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_DTT7592
endif
ifeq "$(DVD_STTUNER_TUNER_TDLB7)" "YES"
	STTUNER_DRV_TER_TUN_TDLB7:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDLB7
endif

ifeq "$(DVD_STTUNER_TUNER_DTT7300X)" "YES"
	STTUNER_DRV_TER_TUN_DTT7300X:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_DTT7300X
endif


ifeq "$(DVD_STTUNER_TUNER_TDQD3)" "YES"
	STTUNER_DRV_TER_TUN_TDQD3:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDQD3
endif
ifeq "$(DVD_STTUNER_TUNER_ENG47402G1)" "YES"
	STTUNER_DRV_TER_TUN_ENG47402G1:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_ENG47402G1
endif
ifeq "$(DVD_STTUNER_TUNER_EAL2780)" "YES"
	STTUNER_DRV_TER_TUN_EAL2780:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_EAL2780
endif
ifeq "$(DVD_STTUNER_TUNER_TDA6650)" "YES"
	STTUNER_DRV_TER_TUN_TDA6650:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDA6650
endif
ifeq "$(DVD_STTUNER_TUNER_TDM1316)" "YES"
	STTUNER_DRV_TER_TUN_TDM1316:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDM1316
endif
ifeq "$(DVD_STTUNER_TUNER_TDEB2)" "YES"
	STTUNER_DRV_TER_TUN_TDEB2:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDEB2
endif
ifeq "$(DVD_STTUNER_TUNER_ED5058)" "YES"
	STTUNER_DRV_TER_TUN_ED5058:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_ED5058
endif
ifeq "$(DVD_STTUNER_TUNER_MIVAR)" "YES"
	STTUNER_DRV_TER_TUN_MIVAR:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_MIVAR
endif
ifeq "$(DVD_STTUNER_TUNER_TDED4)" "YES"
	STTUNER_DRV_TER_TUN_TDED4:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDED4
endif
ifeq "$(DVD_STTUNER_TUNER_DTT7102)" "YES"
	STTUNER_DRV_TER_TUN_DTT7102:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_DTT7102
endif
ifeq "$(DVD_STTUNER_TUNER_TECC2849PG)" "YES"
	STTUNER_DRV_TER_TUN_TECC2849PG:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_TECC2849PG
endif
ifeq "$(DVD_STTUNER_TUNER_TDCC2345)" "YES"
	STTUNER_DRV_TER_TUN_TDCC2345:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDCC2345
endif

ifeq "$(DVD_STTUNER_TUNER_RF4000)" "YES"
	STTUNER_DRV_TER_TUN_RF4000:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_RF4000
endif

ifeq "$(DVD_STTUNER_TUNER_MT2266)" "YES"
	STTUNER_DRV_TER_TUN_MT2266:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_MT2266
endif

ifeq "$(DVD_STTUNER_TUNER_TDTGD108)" "YES"
	STTUNER_DRV_TER_TUN_TDTGD108:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDTGD108
endif
STTUNER_DRV_TER_EXTTUNERS :=1
endif

#--------for ATSC VSB
ifdef STTUNER_USE_ATSC_VSB 
ifeq "$(DVD_STTUNER_TUNER_DTVS223)" "YES"
	STTUNER_DRV_TER_TUN_DTVS223:=1
	CFLAGS += -DSTTUNER_DRV_TER_TUN_DTVS223
	STTUNER_DRV_TER_EXTTUNERS :=1
endif
endif
#------for CABLE
ifdef STTUNER_USE_CAB 
 
ifeq "$(DVD_STTUNER_TUNER_TDBE1)" "YES"
	STTUNER_DRV_CAB_TUN_TDBE1:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDBE1
endif
ifeq "$(DVD_STTUNER_TUNER_TDBE2)" "YES"
	STTUNER_DRV_CAB_TUN_TDBE2:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDBE2
endif
ifeq "$(DVD_STTUNER_TUNER_TDDE1)" "YES"
	STTUNER_DRV_CAB_TUN_TDDE1:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDDE1
endif
ifeq "$(DVD_STTUNER_TUNER_SP5730)" "YES"
	STTUNER_DRV_CAB_TUN_SP5730:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_SP5730
endif
ifeq "$(DVD_STTUNER_TUNER_MT2030)" "YES"
	STTUNER_DRV_CAB_TUN_MT2030:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_MT2030
endif
ifeq "$(DVD_STTUNER_TUNER_MT2040)" "YES"
	STTUNER_DRV_CAB_TUN_MT2040:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_MT2040
endif
ifeq "$(DVD_STTUNER_TUNER_MT2050)" "YES"
	STTUNER_DRV_CAB_TUN_MT2050:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_MT2050
endif
ifeq "$(DVD_STTUNER_TUNER_MT2060)" "YES"
	STTUNER_DRV_CAB_TUN_MT2060:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_MT2060
endif
ifeq "$(DVD_STTUNER_TUNER_DCT7040)" "YES"
	STTUNER_DRV_CAB_TUN_DCT7040:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCT7040

endif
ifeq "$(DVD_STTUNER_TUNER_DCT7045)" "YES"
	STTUNER_DRV_CAB_TUN_DCT7045:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCT7045
endif
ifeq "$(DVD_STTUNER_TUNER_TDQE3)" "YES"
	STTUNER_DRV_CAB_TUN_TDQE3:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDQE3
endif
ifeq "$(DVD_STTUNER_TUNER_DCF8783)" "YES"
	STTUNER_DRV_CAB_TUN_DCF8783:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCF8783
endif
ifeq "$(DVD_STTUNER_TUNER_DCT7045EVAL)" "YES"
	STTUNER_DRV_CAB_TUN_DCT7045EVAL:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCT7045EVAL
endif
ifeq "$(DVD_STTUNER_TUNER_DCT70700)" "YES"
	STTUNER_DRV_CAB_TUN_DCT70700:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCT70700
endif
ifeq "$(DVD_STTUNER_TUNER_TDCHG)" "YES"
	STTUNER_DRV_CAB_TUN_TDCHG:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDCHG
endif

ifeq "$(DVD_STTUNER_TUNER_DCT7050)" "YES"
	STTUNER_DRV_CAB_TUN_DCT7050:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCT7050
endif
ifeq "$(DVD_STTUNER_TUNER_DCT7710)" "YES"
	STTUNER_DRV_CAB_TUN_DCT7710:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCT7710
endif
ifeq "$(DVD_STTUNER_TUNER_DCF8710)" "YES"
	STTUNER_DRV_CAB_TUN_DCF8710:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCF8710
endif
ifeq "$(DVD_STTUNER_TUNER_DCF8720)" "YES"
	STTUNER_DRV_CAB_TUN_DCF8720:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCF8720
endif
ifeq "$(DVD_STTUNER_TUNER_MACOETA50DR)" "YES"
	STTUNER_DRV_CAB_TUN_MACOETA50DR:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_MACOETA50DR
endif
ifeq "$(DVD_STTUNER_TUNER_CD1516LI)" "YES"
	STTUNER_DRV_CAB_TUN_CD1516LI:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_CD1516LI
endif
ifeq "$(DVD_STTUNER_TUNER_DF1CS1223)" "YES"
	STTUNER_DRV_CAB_TUN_DF1CS1223:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DF1CS1223
endif
ifeq "$(DVD_STTUNER_TUNER_SHARPXX)" "YES"
	STTUNER_DRV_CAB_TUN_SHARPXX:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_SHARPXX
endif
ifeq "$(DVD_STTUNER_TUNER_TDBE1X016A)" "YES"
	STTUNER_DRV_CAB_TUN_TDBE1X016A:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDBE1X016A
endif
ifeq "$(DVD_STTUNER_TUNER_TDBE1X601)" "YES"
	STTUNER_DRV_CAB_TUN_TDBE1X60:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDBE1X601
endif
ifeq "$(DVD_STTUNER_TUNER_TDEE4X012A)" "YES"
	STTUNER_DRV_CAB_TUN_TDEE4X012A:=1
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDEE4X012A
endif

endif

#------- for shared
ifdef STTUNER_USE_SHARED

ifeq "$(DVD_STTUNER_TUNER_TD1336)" "YES"
	STTUNER_DRV_SHARED_TUN_TD1336:=1
	CFLAGS += -DSTTUNER_DRV_SHARED_TUN_TD1336
endif
ifeq "$(DVD_STTUNER_TUNER_FQD1236)" "YES"
	STTUNER_DRV_SHARED_TUN_FQD1236:=1
	CFLAGS += -DSTTUNER_DRV_SHARED_TUN_FQD1236
endif
ifeq "$(DVD_STTUNER_TUNER_T2000)" "YES"
	STTUNER_DRV_SHARED_TUN_T2000:=1
	CFLAGS += -DSTTUNER_DRV_SHARED_TUN_T2000
endif
ifeq "$(DVD_STTUNER_TUNER_TDVEH052F)" "YES"
	STTUNER_DRV_SHARED_TUN_TDVEH052F:=1
	CFLAGS += -DSTTUNER_DRV_SHARED_TUN_TDVEH052F
endif
ifeq "$(DVD_STTUNER_TUNER_DTT761X)" "YES"
	STTUNER_DRV_SHARED_TUN_DTT761X:=1
	CFLAGS += -DSTTUNER_DRV_SHARED_TUN_DTT761X
endif

endif

#-----for ALL TUNER
ifeq "$(DVD_STTUNER_DRV_BUILD_ALL_TUNER)" "YES"
   STTUNER_DRV_BUILD_ALL_TUNER := 1
  

ifdef STTUNER_USE_SAT

	CFLAGS += -DSTTUNER_DRV_SAT_TUN_VG1011 -DSTTUNER_DRV_SAT_TUN_TUA6100 -DSTTUNER_DRV_SAT_TUN_EVALMAX

	CFLAGS += -DSTTUNER_DRV_SAT_TUN_S68G21 -DSTTUNER_DRV_SAT_TUN_VG0011 -DSTTUNER_DRV_SAT_TUN_HZ1184 

	CFLAGS += -DSTTUNER_DRV_SAT_TUN_MAX2116 -DSTTUNER_DRV_SAT_TUN_MAX2118 -DSTTUNER_DRV_SAT_TUN_DSF8910 

	CFLAGS += -DSTTUNER_DRV_SAT_TUN_STB6000 -DSTTUNER_DRV_SAT_TUN_STB6100 -DSTTUNER_DRV_SAT_TUN_IX2476 
	
	CFLAGS += -DSTTUNER_DRV_SAT_TUN_STB6110
	
	STTUNER_DRV_SAT_TUN_VG1011 :=1
	STTUNER_DRV_SAT_TUN_TUA6100:=1 
	STTUNER_DRV_SAT_TUN_EVALMAX:=1

	STTUNER_DRV_SAT_TUN_S68G21 :=1
	STTUNER_DRV_SAT_TUN_VG0011 :=1
	STTUNER_DRV_SAT_TUN_HZ1184 :=1

	STTUNER_DRV_SAT_TUN_MAX2116 :=1
	STTUNER_DRV_SAT_TUN_MAX2118 :=1
	STTUNER_DRV_SAT_TUN_DSF8910 :=1

	STTUNER_DRV_SAT_TUN_STB6000 :=1
	STTUNER_DRV_SAT_TUN_STB6100 :=1
	STTUNER_DRV_SAT_TUN_IX2476  :=1
	STTUNER_DRV_SAT_TUN_STB6110 :=1

	
endif

ifdef STTUNER_USE_TER
	CFLAGS += -DSTTUNER_DRV_TER_TUN_DTT7572 -DSTTUNER_DRV_TER_TUN_DTT7578 -DSTTUNER_DRV_TER_TUN_DTT7592

	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDLB7 -DSTTUNER_DRV_TER_TUN_TDQD3 -DSTTUNER_DRV_TER_TUN_ENG47402G1

	CFLAGS += -DSTTUNER_DRV_TER_TUN_EAL2780 -DSTTUNER_DRV_TER_TUN_TDA6650 -DSTTUNER_DRV_TER_TUN_TDM1316

	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDEB2 -DSTTUNER_DRV_TER_TUN_ED5058 -DSTTUNER_DRV_TER_TUN_MIVAR  

	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDED4 -DSTTUNER_DRV_TER_TUN_DTT7102 -DSTTUNER_DRV_TER_TUN_TECC2849PG 

	CFLAGS += -DSTTUNER_DRV_TER_TUN_TDCC2345 -DSTTUNER_DRV_TER_TUN_RF4000 -DSTTUNER_DRV_TER_TUN_TDTGD108

	CFLAGS += -DSTTUNER_DRV_TER_TUN_DTT7300X
	
	STTUNER_DRV_TER_TUN_DTT7572:=1
	STTUNER_DRV_TER_TUN_DTT7578:=1
	STTUNER_DRV_TER_TUN_DTT7592:=1
	STTUNER_DRV_TER_TUN_TDLB7 :=1
	STTUNER_DRV_TER_TUN_TDQD3 :=1
	STTUNER_DRV_TER_TUN_ENG47402G1:=1

	STTUNER_DRV_TER_TUN_EAL2780 :=1
	STTUNER_DRV_TER_TUN_TDA6650 :=1
	STTUNER_DRV_TER_TUN_TDM1316:=1

	STTUNER_DRV_TER_TUN_TDEB2 :=1
	STTUNER_DRV_TER_TUN_ED5058 :=1
	STTUNER_DRV_TER_TUN_MIVAR  :=1

	STTUNER_DRV_TER_TUN_TDED4 :=1
	STTUNER_DRV_TER_TUN_DTT7102 :=1
	STTUNER_DRV_TER_TUN_TECC2849PG :=1

	STTUNER_DRV_TER_TUN_TDCC2345 :=1
	
	STTUNER_DRV_TER_TUN_RF4000:=1
	STTUNER_DRV_TER_TUN_TDTGD108:=1
	
	STTUNER_DRV_TER_TUN_DTT7300X:=1
endif
ifeq "$(DVD_STTUNER_USE_ATSC_VSB)" "YES"
CFLAGS += -DSTTUNER_DRV_TER_TUN_DTVS223 -DSTTUNER_DRV_SHARED_TUN_DTT7600 -DSTTUNER_DRV_SHARED_TUN_DTT768XX -DSTTUNER_DRV_TER_TUN_RF4000
STTUNER_DRV_TER_TUN_DTVS223:=1
STTUNER_DRV_SHARED_TUN_DTT7600:=1
STTUNER_DRV_SHARED_TUN_DTT768XX:=1
STTUNER_DRV_TER_TUN_RF4000:=1
STTUNER_USE_SHARED:=1
STTUNER_DRV_TER_EXTTUNERS:=1
STTUNER_DRV_SHARED_EXTTUNERS:=1
endif
ifdef STTUNER_USE_CAB

    CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDBE1 -DSTTUNER_DRV_CAB_TUN_TDBE2 -DSTTUNER_DRV_CAB_TUN_TDDE1  

	CFLAGS += -DSTTUNER_DRV_CAB_TUN_SP5730 -DSTTUNER_DRV_CAB_TUN_MT2030 -DSTTUNER_DRV_CAB_TUN_MT2040 

	CFLAGS += -DSTTUNER_DRV_CAB_TUN_MT2050 -DSTTUNER_DRV_CAB_TUN_MT2060 -DSTTUNER_DRV_CAB_TUN_DCT7040   

	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCT7050 -DSTTUNER_DRV_CAB_TUN_DCT7710 -DSTTUNER_DRV_CAB_TUN_DCF8710  

	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCF8720 -DSTTUNER_DRV_CAB_TUN_MACOETA50DR -DSTTUNER_DRV_CAB_TUN_CD1516LI

	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DF1CS1223 -DSTTUNER_DRV_CAB_TUN_SHARPXX -DSTTUNER_DRV_CAB_TUN_TDBE1X016A

	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDBE1X601 -DSTTUNER_DRV_CAB_TUN_TDEE4X012A -DSTTUNER_DRV_CAB_TUN_DCT7045
	
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDQE3 -DSTTUNER_DRV_CAB_TUN_DCF8783 -DSTTUNER_DRV_CAB_TUN_DCT7045EVAL
	
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_DCT70700 -DSTTUNER_DRV_CAB_TUN_TDCHG -DSTTUNER_DRV_CAB_TUN_TDCJG
	
	CFLAGS += -DSTTUNER_DRV_CAB_TUN_TDCGG
	
	STTUNER_DRV_CAB_TUN_TDBE1 :=1
	STTUNER_DRV_CAB_TUN_TDBE2 :=1
	STTUNER_DRV_CAB_TUN_TDDE1  :=1

	STTUNER_DRV_CAB_TUN_SP5730 :=1
	STTUNER_DRV_CAB_TUN_MT2030 :=1
	STTUNER_DRV_CAB_TUN_MT2040 :=1

	STTUNER_DRV_CAB_TUN_MT2050 :=1
	STTUNER_DRV_CAB_TUN_MT2060 :=1
	STTUNER_DRV_CAB_TUN_DCT7040 :=1
	
	STTUNER_DRV_CAB_TUN_DCT7045 :=1
	STTUNER_DRV_CAB_TUN_TDQE3 :=1
	STTUNER_DRV_CAB_TUN_DCF8783 :=1
	
	STTUNER_DRV_CAB_TUN_DCT7045EVAL :=1
	STTUNER_DRV_CAB_TUN_DCT70700 :=1
	STTUNER_DRV_CAB_TUN_TDCHG :=1
	
	STTUNER_DRV_CAB_TUN_TDCJG :=1
	STTUNER_DRV_CAB_TUN_TDCGG :=1

	STTUNER_DRV_CAB_TUN_DCT7050 :=1
	STTUNER_DRV_CAB_TUN_DCT7710 :=1
	STTUNER_DRV_CAB_TUN_DCF8710  :=1

	STTUNER_DRV_CAB_TUN_DCF8720 :=1
	STTUNER_DRV_CAB_TUN_MACOETA50DR :=1
	STTUNER_DRV_CAB_TUN_CD1516LI:=1
	
	STTUNER_DRV_CAB_TUN_DF1CS1223 :=1
	STTUNER_DRV_CAB_TUN_SHARPXX :=1
	STTUNER_DRV_CAB_TUN_TDBE1X016A:=1
	
	STTUNER_DRV_CAB_TUN_TDBE1X601 :=1
	STTUNER_DRV_CAB_TUN_TDEE4X012A:=1


endif
ifdef STTUNER_USE_SHARED
	CFLAGS += -DSTTUNER_DRV_SHARED_TUN_TD1336
STTUNER_DRV_SHARED_TUN_TD1336:=1
	CFLAGS += -DSTTUNER_DRV_SHARED_TUN_FQD1236
STTUNER_DRV_SHARED_TUN_FQD1236:=1
	CFLAGS += -DSTTUNER_DRV_SHARED_TUN_T2000
STTUNER_DRV_SHARED_TUN_T2000:=1
CFLAGS += -DSTTUNER_DRV_SHARED_TUN_MT2131
STTUNER_DRV_SHARED_TUN_MT2131:=1
	CFLAGS += -DSTTUNER_DRV_SHARED_TUN_TDVEH052F
STTUNER_DRV_SHARED_TUN_TDVEH052F:=1
CFLAGS += -DSTTUNER_DRV_SHARED_TUN_DTT761X
STTUNER_DRV_SHARED_TUN_DTT761X:=1
endif

endif


# --------------------------------------------------------------------

#---------------------STTUNER TOP LEVEL NUM HANDLES ---------------------

ifdef DVD_STTUNER_NUM_MAX_HANDLES
CFLAGS += -DSTTUNER_NUM_MAX_HANDLES=$(DVD_STTUNER_NUM_MAX_HANDLES)  
endif


#ifdef STTUNER_MINIDRIVER
#CFLAGS += -DSTTUNER_NUM_MAX_HANDLES=1  
#endif

# --------------------------------------------------------------------

# For GNBvd25440 --> Allow override of task priorities. These priorities if
# not defined in the  build environment, have default values set in the
# files cabtask.h, sattask.h & tertask.h

ifdef STTUNER_SCAN_CAB_TASK_PRIORITY
  CFLAGS += -DSTTUNER_SCAN_CAB_TASK_PRIORITY=$(STTUNER_SCAN_CAB_TASK_PRIORITY)
endif

ifdef STTUNER_SCAN_SAT_TASK_PRIORITY
  CFLAGS += -DSTTUNER_SCAN_SAT_TASK_PRIORITY=$(STTUNER_SCAN_SAT_TASK_PRIORITY)
endif

ifdef STTUNER_SCAN_TER_TASK_PRIORITY
  CFLAGS += -DSTTUNER_SCAN_TER_TASK_PRIORITY=$(STTUNER_SCAN_TER_TASK_PRIORITY)
endif

#Quiet mode support for DTV
ifeq "$(DVD_STTUNER_DEBUG_DTV_I2C_QUIET_MODE)" "YES"
  CFLAGS += -DSTTUNER_DEBUG_DTV_I2C_QUIET_MODE
endif

# --------------------------------------------------------------------
# top level API

INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX). $(INCLUDE_PREFIX)./include
OBJS = ./manager/sttuner.tco ./manager/sysdbase.tco

ifndef STTUNER_MINIDRIVER
OBJS += ./manager/util.tco ./manager/tunervalidate.tco \
        ./manager/swioctl.tco    

ifdef STTUNER_DRV_SAT_STV0299
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif

ifdef STTUNER_DRV_SAT_STB0899
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif


ifdef STTUNER_DRV_SAT_STX0288
OBJS += ./drivers/chip.tco
endif

ifdef STTUNER_DRV_SAT_LNBH21
OBJS += ./drivers/chip.tco
endif

ifdef STTUNER_DRV_SAT_LNBH24
OBJS += ./drivers/chip.tco
endif
ifdef STTUNER_DRV_SAT_LNBH23
OBJS += ./drivers/chip.tco
endif

ifdef STTUNER_DRV_SAT_LNB21
OBJS += ./drivers/ioreg.tco
endif

endif

 
        
        
# --------------------------------------------------------------------
# top level driver

INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX)./drivers/include
ifndef STTUNER_MINIDRIVER
ifdef STTUNER_DRV_SAT_STV0399E
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif

ifdef STTUNER_DRV_SAT_STV0399
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif
endif

ifndef STTUNER_MINIDRIVER
ifdef STTUNER_DRV_TER_STV0360
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif
endif

ifdef STTUNER_DRV_TER_STV0361
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif

ifdef STTUNER_DRV_TER_STV0362
OBJS += ./drivers/chip.tco
endif

ifdef STTUNER_DRV_TER_STB0370VSB
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif

ifdef STTUNER_DRV_TER_STV0372
OBJS += ./drivers/chip.tco
endif

ifdef STTUNER_DRV_CAB_STV0297
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif

ifdef STTUNER_DRV_CAB_STV0297J
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif

ifdef STTUNER_DRV_CAB_STV0297E
OBJS += ./drivers/chip.tco
endif

ifdef STTUNER_DRV_CAB_STB0370QAM
OBJS += ./drivers/ioreg.tco ./drivers/ioarch.tco
endif

     

#INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX)./drivers/include
ifndef STTUNER_MINIDRIVER
OBJS += ./drivers/drivers.tco ./drivers/iocom.tco ./drivers/chip.tco
endif




ifdef STTUNER_MINIDRIVER  
OBJS += ./drivers/ioreg.tco  ./drivers/iodirect.tco
endif



# enable stapi includes for chip.h
CFLAGS += -DCHIP_STAPI

# --------------------------------------------------------------------
# shared drivers

ifdef STTUNER_USE_SHARED
 INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX)./drivers/shared/include
 CFLAGS += -DSTTUNER_USE_SHARED
 ifndef STTUNER_MINIDRIVER
 OBJS += ./drivers/shared/sharedrv.tco
 endif

 # -- null --
 ifdef STTUNER_DRV_SHARED_NULL
  CFLAGS += -DSTTUNER_DRV_SHARED_NULL
  ifndef STTUNER_MINIDRIVER
  OBJS += ./drivers/shared/demod/none/dnone.tco ./drivers/shared/tuner/tnone.tco
  endif
 endif
 # -- external tuner ---
 ifdef STTUNER_DRV_SHARED_EXTTUNERS
  CFLAGS += -DSTTUNER_DRV_SHARED_EXTTUNERS
  ifndef STTUNER_MINIDRIVER
  OBJS += ./drivers/shared/tuner/tunshdrv.tco 
  endif
 endif

endif #if STTUNER_USE_SHARED

# --------------------------------------------------------------------
# satellite

ifdef STTUNER_USE_SAT
 INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX)./drivers/sat/include
ifndef STTUNER_MINIDRIVER 
OBJS += ./drivers/sat/sdrv.tco
endif

 CFLAGS += -DSTTUNER_USE_SAT
 INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX)./include/sat
ifndef STTUNER_MINIDRIVER 
OBJS += ./manager/sat/satutil.tco \
         ./manager/sat/init.tco    \
         ./manager/sat/term.tco    \
         ./manager/sat/open.tco    \
         ./manager/sat/close.tco   \
         ./manager/sat/ioctl.tco   \
         ./manager/sat/sattask.tco \
         ./manager/sat/scan.tco    \
         ./manager/sat/get.tco     \
         ./manager/sat/put.tco
endif
ifdef STTUNER_MINIDRIVER
OBJS += ./manager/sat/init.tco    \
         ./manager/sat/term.tco    \
         ./manager/sat/open.tco    \
         ./manager/sat/close.tco   \
         ./manager/sat/sattask.tco \
         ./manager/sat/get.tco     \
         ./manager/sat/put.tco
endif
 # -- sat null drivers --
 ifdef STTUNER_DRV_SAT_NULL
  CFLAGS += -DSTTUNER_DRV_SAT_NULL
  OBJS += ./drivers/sat/lnb/lnone.tco
 endif

 # -- sat stv0299 driver set --
 ifdef STTUNER_DRV_SAT_STV0299
  CFLAGS += -DSTTUNER_DRV_SAT_STV0299
ifndef STTUNER_MINIDRIVER 
  OBJS += ./drivers/sat/demod/stv0299/reg0299.tco \
          ./drivers/sat/demod/stv0299/drv0299.tco \
          ./drivers/sat/demod/stv0299/d0299.tco
endif
  
ifdef STTUNER_MINIDRIVER 
OBJS += ./drivers/sat/demod/stv0299/drv0299.tco \
          ./drivers/sat/demod/stv0299/d0299.tco
endif
 endif
 
 # -- sat stx0288 driver set --
 ifdef STTUNER_DRV_SAT_STX0288
  CFLAGS += -DSTTUNER_DRV_SAT_STX0288
  OBJS += ./drivers/sat/demod/stx0288/drv0288.tco \
          ./drivers/sat/demod/stx0288/d0288.tco
   endif

# -- sat stv0399 driver set --
 
 
 ifdef STTUNER_DRV_SAT_STB0899
  CFLAGS += -DSTTUNER_DRV_SAT_STB0899
  OBJS += ./drivers/sat/demod/stb0899/d0899.tco \
          ./drivers/sat/demod/stb0899/drv0899.tco \
          ./drivers/sat/demod/stb0899/d0899_dvbs2util.tco\
          ./drivers/sat/demod/stb0899/d0899_util.tco
  endif
 
 ifdef STTUNER_DRV_SAT_STV0399  
  
  CFLAGS += -DSTTUNER_DRV_SAT_STV0399
  CFLAGS += -DSTTUNER_DRV_SAT_STV0399E
  OBJS += ./drivers/sat/demod/stv0399/drv399E.tco \
          ./drivers/sat/demod/stv0399/d0399E.tco
 endif

 
 # -- if lnb21 driver is to be used --
 ifdef STTUNER_DRV_SAT_LNB21
  CFLAGS += -DSTTUNER_DRV_SAT_LNB21
  OBJS += ./drivers/sat/lnb/lnb21.tco
 endif
   
  ifdef STTUNER_DRV_SAT_LNBH21
  CFLAGS += -DSTTUNER_DRV_SAT_LNBH21
  OBJS += ./drivers/sat/lnb/lnbh21.tco
 endif
 
  ifdef STTUNER_DRV_SAT_LNBH24
  CFLAGS += -DSTTUNER_DRV_SAT_LNBH24
  OBJS += ./drivers/sat/lnb/lnbh24.tco
 endif
 
   ifdef STTUNER_DRV_SAT_LNBH23
  CFLAGS += -DSTTUNER_DRV_SAT_LNBH23
  OBJS += ./drivers/sat/lnb/lnbh24.tco
 endif
   
 ifdef STTUNER_LNB_POWER_THRU_BACKENDGPIO
   OBJS += ./drivers/sat/lnb/lnbBEIO.tco
 endif
 
 ifdef STTUNER_LNB_POWER_THRU_DEMODIO
   OBJS += ./drivers/sat/lnb/lnbIO.tco
   OBJS += ./drivers/ioreg.tco
 endif
  
 # -- if scr driver is to be used --
 ifdef STTUNER_DRV_SAT_SCR
  OBJS += ./drivers/sat/scr/scrdrv.tco
 endif
   
 # --- if diseqc driver is to be used --
 ifdef STTUNER_DRV_SAT_DISEQC_THROUGH_BACKEND
 OBJS += ./drivers/sat/diseqc/diseqcdrv.tco
 endif 
   
 # -- sat external (ZIF) tuner device set (in one file) --
 ifdef STTUNER_DRV_SAT_STV0299
  OBJS += ./drivers/sat/tuner/tunsdrv.tco
 endif
 
 ifdef STTUNER_DRV_SAT_STX0288
  OBJS += ./drivers/sat/tuner/tunsdrv.tco
 endif
 
 ifdef STTUNER_DRV_SAT_STB0899
  OBJS += ./drivers/sat/tuner/tunsdrv.tco
 endif

 # -- sat stem lnb/tuners --
 ifdef STTUNER_DRV_SAT_STEM
  CFLAGS += -DSTTUNER_DRV_SAT_STEM
  OBJS += ./drivers/sat/lnb/lstem.tco
  endif
  

 # -- defined for dual stem --
 ifdef STTUNER_DRV_SAT_DUAL_STEM_299
  CFLAGS += -DSTTUNER_DRV_SAT_DUAL_STEM_299
 endif

 # -- defined for futarque board --
 ifdef STTUNER_DRV_SAT_F0299
  CFLAGS += -DSTTUNER_DRV_SAT_F0299
 endif

endif #if STTUNER_USE_SAT

# --------------------------------------------------------------------
# cable

ifdef STTUNER_USE_CAB
 INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX)./drivers/cab/include
 OBJS += ./drivers/cab/cdrv.tco

 CFLAGS += -DSTTUNER_USE_CAB
 INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX)./include/cab
 OBJS += ./manager/cab/cabutil.tco \
         ./manager/cab/init.tco    \
         ./manager/cab/term.tco    \
         ./manager/cab/open.tco    \
         ./manager/cab/close.tco   \
         ./manager/cab/ioctl.tco   \
         ./manager/cab/cabtask.tco \
         ./manager/cab/scan.tco    \
         ./manager/cab/get.tco     \
         ./manager/cab/put.tco

 # -- cab stv0297 driver set --
 ifdef STTUNER_DRV_CAB_STV0297
  CFLAGS += -DSTTUNER_DRV_CAB_STV0297
  OBJS += ./drivers/cab/demod/stv0297/reg0297.tco \
          ./drivers/cab/demod/stv0297/d0297.tco \
          ./drivers/cab/demod/stv0297/drv0297.tco
 endif

  # -- cab stv0297e driver set --
 ifdef STTUNER_DRV_CAB_STV0297E
  CFLAGS += -DSTTUNER_DRV_CAB_STV0297E
  OBJS += ./drivers/cab/demod/stv0297e/d0297e.tco \
          ./drivers/cab/demod/stv0297e/drv0297e.tco
 endif


# -- cab stv0297J driver set --
 ifdef STTUNER_DRV_CAB_STV0297J
  CFLAGS += -DSTTUNER_DRV_CAB_STV0297J
  ifdef STTUNER_DRV_CAB_J83A
   CFLAGS += -DSTTUNER_DRV_CAB_J83A
  endif
  ifdef STTUNER_DRV_CAB_J83B
   CFLAGS += -DSTTUNER_DRV_CAB_J83B
  endif
  ifdef STTUNER_DRV_CAB_J83C
   CFLAGS += -DSTTUNER_DRV_CAB_J83C
  endif
  ifdef STTUNER_DRV_CAB_DOCSIS_FE
   CFLAGS += -DSTTUNER_DRV_CAB_DOCSIS_FE
  endif
  ifdef STTUNER_DRV_CAB_CBR0
   CFLAGS += -DSTTUNER_DRV_CAB_CBR0
  endif
  ifdef STTUNER_DRV_CAB_CBR1200
   CFLAGS += -DSTTUNER_DRV_CAB_CBR1200
  endif
  OBJS += ./drivers/cab/demod/stv0297j/reg0297j.tco \
          ./drivers/cab/demod/stv0297j/d0297j.tco \
          ./drivers/cab/demod/stv0297j/drv0297j.tco
 endif

  ifdef STTUNER_DRV_CAB_STB0370QAM
  CFLAGS += -DSTTUNER_DRV_CAB_STB0370QAM
  ifdef STTUNER_DRV_CAB_J83B
   CFLAGS += -DSTTUNER_DRV_CAB_J83B
  endif
 
  OBJS += ./drivers/cab/demod/stb0370qam/reg0370qam.tco \
          ./drivers/cab/demod/stb0370qam/d0370qam.tco \
          ./drivers/cab/demod/stb0370qam/drv0370qam.tco
 endif
 # -- cab external tuner device set (in one file) --
 ifdef STTUNER_DRV_CAB_EXTTUNERS
  CFLAGS += -DSTTUNER_DRV_CAB_EXTTUNERS
  OBJS += ./drivers/cab/tuner/tcdrv.tco
 endif

endif #if STTUNER_USE_CAB

# --------------------------------------------------------------------
# terrestrial

ifdef STTUNER_USE_TER_PLUS_ATSC_VSB  
 INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX)./drivers/ter/include
 ifndef STTUNER_MINIDRIVER 
 OBJS += ./drivers/ter/tdrv.tco
 endif

ifdef STTUNER_USE_TER
 CFLAGS += -DSTTUNER_USE_TER
endif
ifdef STTUNER_USE_ATSC_VSB
 CFLAGS += -DSTTUNER_USE_ATSC_VSB
endif

 INCLUDES:= $(INCLUDES) $(INCLUDE_PREFIX)./include/ter
ifndef STTUNER_MINIDRIVER 
 OBJS += ./manager/ter/terutil.tco \
         ./manager/ter/init.tco    \
	 ./manager/ter/term.tco    \
         ./manager/ter/open.tco    \
         ./manager/ter/close.tco   \
         ./manager/ter/ioctl.tco   \
         ./manager/ter/tertask.tco \
         ./manager/ter/scan.tco    \
         ./manager/ter/get.tco     \
         ./manager/ter/put.tco
endif
ifdef STTUNER_MINIDRIVER 
 OBJS += ./manager/ter/init.tco    \
         ./manager/ter/term.tco    \
         ./manager/ter/open.tco    \
         ./manager/ter/close.tco   \
         ./manager/ter/tertask.tco \
         ./manager/ter/get.tco     \
         ./manager/ter/put.tco
endif

 # -- ter stv0360 driver set --
 ifndef STTUNER_MINIDRIVER
 ifdef STTUNER_DRV_TER_STV0360
  CFLAGS += -DSTTUNER_DRV_TER_STV0360
  OBJS += ./drivers/ter/demod/stv0360/d0360.tco \
          ./drivers/ter/demod/stv0360/d0360_drv.tco \
          ./drivers/ter/demod/stv0360/d0360_echo.tco
 endif
 endif
 # --ter 360 mini driver set--
 ifdef STTUNER_MINIDRIVER
 ifdef STTUNER_DRV_TER_STV0360
  CFLAGS += -DSTTUNER_DRV_TER_STV0360
  OBJS += ./drivers/ter/demod/stv0360/d0360_init.tco \
          ./drivers/ter/demod/stv0360/d0mini360.tco \
          ./drivers/ter/demod/stv0360/d0360minidrv.tco \
          ./drivers/ter/demod/stv0360/d0360miniecho.tco
 endif
 endif
 
 # -- ter stv0361 driver set --
 ifdef STTUNER_DRV_TER_STV0361
  CFLAGS += -DSTTUNER_DRV_TER_STV0361
  OBJS += ./drivers/ter/demod/stv0361/d0361.tco \
          ./drivers/ter/demod/stv0361/d0361_drv.tco \
          ./drivers/ter/demod/stv0361/d0361_echo.tco
 endif
 
 # -- ter stv0362 driver set --
 ifdef STTUNER_DRV_TER_STV0362
  CFLAGS += -DSTTUNER_DRV_TER_STV0362
  OBJS += ./drivers/ter/demod/stv0362/d0362.tco \
          ./drivers/ter/demod/stv0362/d0362_drv.tco
          
 endif

  # -- ter stb0370VSB driver set --
 ifdef STTUNER_DRV_TER_STB0370VSB
  CFLAGS += -DSTTUNER_DRV_TER_STB0370VSB
  OBJS += ./drivers/ter/demod/stb0370vsb/d0370VSB.tco \
          ./drivers/ter/demod/stb0370vsb/d0370VSB_util.tco \
          ./drivers/ter/demod/stb0370vsb/d0370VSB_drv.tco   
 endif
 
  # -- ter stv0372 driver set --
 ifdef STTUNER_DRV_TER_STV0372
  CFLAGS += -DSTTUNER_DRV_TER_STV0372
  OBJS += ./drivers/ter/demod/stv0372/d0372.tco \
          ./drivers/ter/demod/stv0372/d0372_util.tco \
          ./drivers/ter/demod/stv0372/d0372_drv.tco   
 endif
 # -- ter external tuner 360 -----
ifdef STTUNER_DRV_TER_STV0360
  OBJS += ./drivers/ter/tuner/tuntdrv.tco \
          ./drivers/ter/tuner/ll_tun0360.tco 
          
          ifdef STTUNER_DRV_TER_TUN_RF4000 
  OBJS += ./drivers/ter/tuner/RF4KCCLib.tco
          endif
          
             ifdef STTUNER_DRV_TER_TUN_MT2266 
  OBJS += ./drivers/ter/tuner/mt2266.tco
endif 

endif 

 # -- ter external tuner 361 -----
ifdef STTUNER_DRV_TER_STV0361
  OBJS += ./drivers/ter/tuner/tuntdrv.tco \
          ./drivers/ter/tuner/ll_tun0360.tco 
ifdef STTUNER_DRV_TER_TUN_RF4000 
  OBJS += ./drivers/ter/tuner/RF4KCCLib.tco
endif
          
   ifdef STTUNER_DRV_TER_TUN_MT2266 
  OBJS += ./drivers/ter/tuner/mt2266.tco
endif 

          
endif  

# -- ter external tuner 362 -----
ifdef STTUNER_DRV_TER_STV0362
  ifndef STTUNER_DRV_SHARED_EXTTUNERS
ifdef STTUNER_DRV_TER_EXTTUNERS
  OBJS += ./drivers/ter/tuner/tuntdrv.tco \
          ./drivers/ter/tuner/ll_tun0360.tco
endif
 endif	  
ifdef STTUNER_DRV_TER_TUN_RF4000 
     OBJS += ./drivers/ter/tuner/RF4KCCLib.tco   
endif
          
   ifdef STTUNER_DRV_TER_TUN_MT2266 
  OBJS += ./drivers/ter/tuner/mt2266.tco
endif
 

ifdef STTUNER_DRV_SHARED_EXTTUNERS

ifdef STTUNER_DRV_SHARED_TUN_MT2131
  OBJS += ./drivers/shared/tuner/mt2131.tco
endif             
endif      
          
endif        

 # -- ter external tuner 370 -----
ifdef STTUNER_DRV_TER_STB0370VSB
ifdef STTUNER_DRV_TER_EXTTUNERS
  OBJS += ./drivers/ter/tuner/tuntdrv.tco \
          ./drivers/ter/tuner/ll_tun0360.tco 
endif          
ifdef STTUNER_DRV_TER_TUN_RF4000 
     OBJS += ./drivers/ter/tuner/RF4KCCLib.tco   
endif 
 ifdef STTUNER_DRV_SHARED_TUN_MT2131
  OBJS += ./drivers/shared/tuner/mt2131.tco
endif                
        
endif
 
 # -- ter external tuner 372 -----
ifdef STTUNER_DRV_TER_STV0372
ifdef STTUNER_DRV_TER_EXTTUNERS
  OBJS += ./drivers/ter/tuner/tuntdrv.tco \
          ./drivers/ter/tuner/ll_tun0360.tco 
endif
 ifdef STTUNER_DRV_TER_TUN_RF4000 
     OBJS += ./drivers/ter/tuner/RF4KCCLib.tco   
endif  
ifdef STTUNER_DRV_SHARED_TUN_MT2131
  OBJS += ./drivers/shared/tuner/mt2131.tco
endif          

         
endif

endif #STTUNER_USE_TER_PLUS_ATSC_VSB
ifdef STTUNER_USE_TER_PLUS_ATSC_VSB
ifdef STTUNER_DRV_BUILD_ALL_TUNER
OBJS += ./drivers/ter/tuner/tuntdrv.tco \
        ./drivers/ter/tuner/ll_tun0360.tco 
endif
endif

# --------------------------------------------------------------------
# debug support

#reporting via STTBX (must enable print/report to see ANY debug output)
#CFLAGS += -DSTTBX_REPORT -DSTTBX_PRINT
#CFLAGS += -DSTTBX_INPUT

#top level
#CFLAGS += -DSTTUNER_DEBUG_MODULE_UTIL
#CFLAGS += -DSTTUNER_DEBUG_MODULE_STTUNER
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SWIOCTL
#CFLAGS += -DSTTUNER_DEBUG_MODULE_VALIDATE

#satellite device
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_INIT
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_OPEN
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_CLOSE
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_TERM
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_UTILS
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_IOCTL
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_SATTASK
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_SCAN
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_GET
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SAT_PUT

#cable device
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_INIT
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_OPEN
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_CLOSE
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_TERM
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_UTILS
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_IOCTL
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_CABTASK
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_SCAN
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_GET
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CAB_PUT

#terrestrial device
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_INIT
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_OPEN
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_CLOSE
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_TERM
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_UTILS
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_IOCTL
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_TERTASK
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_SCAN
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_GET
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERR_PUT
#CFLAGS += -DSTTUNER_TERR_TMM_6MHZ
#CFLAGS += -DSTTUNER_TERR_TMM_7MHZ
#CFLAGS += -DSTTUNER_TERR_TMM_8MHZ

#driver install and I/O
#CFLAGS += -DSTTUNER_DEBUG_MODULE_DRIVERS
#CFLAGS += -DSTTUNER_DEBUG_MODULE_IOARCH
#CFLAGS += -DSTTUNER_DEBUG_MODULE_IOREG
#CFLAGS += -DSTTUNER_DEBUG_MODULE_IOREG_ADD
#CFLAGS += -DSTTUNER_DEBUG_MODULE_IOREG_RESET
#CFLAGS += -DSTTUNER_DEBUG_MODULE_CHIP

#shared drivers
# CFLAGS += -DSTTUNER_DEBUG_MODULE_SHARED
# CFLAGS += -DSTTUNER_DEBUG_MODULE_SHARED_DNONE
# CFLAGS += -DSTTUNER_DEBUG_MODULE_SHARED_TNONE

#satellite drivers
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SDRV
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_D0299
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_D0288
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_REG0299
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_DRV0299
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_L0299
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_LNONE
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_D0399
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_L0399
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_LLA0399
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_TUNSDRV
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_LSTEM
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_LNB21
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_LNBH21
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_SCR
#CFLAGS += -DSTTUNER_DEBUG_MODULE_SATDRV_DISEQC

#cable drivers
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CDRV
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_D0297
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_REG0297
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_DRV0297
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_D0297J
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_REG0297J
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_DRV0297J
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_D0297E
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_DRV0297E
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_D0370QAM
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_REG0370QAM
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_DRV0370QAM
# CFLAGS += -DSTTUNER_DEBUG_MODULE_CABDRV_TCDRV

#terrestrial drivers
# CFLAGS += -DSTTUNER_DEBUG_MODULE_TDRV
# CFLAGS += -DSTTUNER_DEBUG_MODULE_TERDRV_D0360
# CFLAGS += -DSTTUNER_DEBUG_MODULE_TERDRV_D0361
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERDRV_D0362
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERDRV_D0362_DRV
#CFLAGS += -DSTTUNER_DEBUG_MODULE_TERDRV_D0362_DRV_LOCKSTAGES
# CFLAGS += -DSTTUNER_DEBUG_MODULE_TERDRV_TUNTDRV
# CFLAGS += -DSTTUNER_DEBUG_MODULE_TERDRV_D0370VSB
# CFLAGS += -DSTTUNER_DEBUG_MODULE_TERDRV_D0372



#common options for ter sat cab
#CFLAGS += -DSTTUNER_DEBUG_LOCK_STAGES

# --------------------------------------------------------------------
# other build options

# STi5508 I2C bug means we need SW start/stop version of i2c driver
ifeq ($(DVD_FRONTEND), 5508)
 export DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP
endif

# software START/STOP is also supported on 5518
ifeq ($(DVD_FRONTEND),5518)
 export DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP
endif

# --------------------------------------------------------------------
# Top level make options for sttuner:
#
# 1/ make                makes sttuner.lib
# 2/ make clean          clean tree for sttuner.lib but not unitary test branch.
# 3/ make clean_sttuner  clean every branch in sttuner tree including unitary test branch.
# 4/ make sttuner        makes sttuner.lib then unitary test(s)
# 5/ remake_sttuner:     Performs 'make clean_sttuner' then 'make sttuner'
#
# Lower level cleans
#
# 1/ make clean_sttuner_lib         Force rebuilding of sttuner.lib from *.tco components
# 2/ make clean_sttuner_drv         I/O components.
# 3/ make clean_sttuner_drv_sat     Satellite drivers
# 4/ make clean_sttuner_drv_cab     Cable drivers
# 5/ make clean_sttuner_drv_ter     Terrestrial drivers
# 6/ make clean_sttuner_drv_shared  Shared drivers
# 7/ make clean_sttuner_mgr         Satellite, Cable & terrestrial device managers.

ST20_OBJS := $(OBJS)
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(OBJS))
# Default rules
include $(DVD_MAKE)/defrules.mak


$(LIB_PREFIX)sttuner$(LIB_SUFFIX): $($(ARCHITECTURE)_OBJS)
	@-$(RM) *$(LIB_SUFFIX)
	$(BUILD_LIBRARY)
	@-$(MKDIR) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@$(CP) *$(LIB_SUFFIX) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	@-$(CP) *$(LIB_SUFFIX) $(DVD_EXPORTS)

# -- clean commands ---

# This is a quick-fix for getting the clean on a PC to work.
# The single \ was being interpreted as an escape char.
ifeq "$(DVD_HOST)" "pc"
  SLASHES := $(GOOD_SLASH)$(GOOD_SLASH)
else
  SLASHES := $(GOOD_SLASH)
endif

ifeq "$(ARCHITECTURE)" "ST20"
clean_sttuner_lib:
	$(LIBCLEAN)
	$(RM) *.lib
	-$(FULLRM) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	-$(RMDIR) objs

clean_sttuner_drv:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/*.tco);

clean_sttuner_drv_sat:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stv0299/*.tco);
	
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stv0399/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stb0899/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stx0288/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/tuner/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/lnb/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/scr/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/diseqc/*.tco);

clean_sttuner_drv_cab:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stv0297/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stv0297j/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stv0297e/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stb0370qam/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/tuner/*.tco);

clean_sttuner_drv_ter:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0360/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0361/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0362/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stb0370vsb/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0372/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/tuner/*.tco);

clean_sttuner_drv_shared:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/shared/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/shared/demod/none/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/shared/tuner/*.tco);

clean_sttuner_mgr:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/sat/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/cab/*.tco);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/ter/*.tco);
endif
ifeq "$(ARCHITECTURE)" "ST40"
clean_sttuner_lib:
	$(LIBCLEAN)
	$(RM) *.a
	-$(FULLRM) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	-$(RMDIR) objs

clean_sttuner_drv:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/*.o);

clean_sttuner_drv_sat:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stv0299/*.o);
	
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stv0399/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stb0899/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stx0288/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/tuner/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/lnb/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/scr/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/diseqc/*.o);

clean_sttuner_drv_cab:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stv0297/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stv0297j/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stv0297e/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stb0370qam/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/tuner/*.o);

clean_sttuner_drv_ter:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0360/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0361/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0362/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stb0370vsb/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0372/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/tuner/*.o);

clean_sttuner_drv_shared:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/shared/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/shared/demod/none/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/shared/tuner/*.o);

clean_sttuner_mgr:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/sat/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/cab/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/ter/*.o);
endif
ifeq "$(ARCHITECTURE)" "ST200"
clean_sttuner_lib:
	$(LIBCLEAN)
	$(RM) *.a
	-$(FULLRM) objs$(GOOD_SLASH)$(OBJECT_DIRECTORY)
	-$(RMDIR) objs

clean_sttuner_drv:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/*.o);

clean_sttuner_drv_sat:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stv0299/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stx0288/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stv0399/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/demod/stb0899/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/tuner/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/lnb/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/scr/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/sat/diseqc/*.o);

clean_sttuner_drv_cab:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stv0297/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stv0297j/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stv0297e/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/stb0370qam/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/demod/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/cab/tuner/*.o);

clean_sttuner_drv_ter:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0360/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0361/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0362/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stb0370vsb/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/stv0372/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/demod/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/ter/tuner/*.o);

clean_sttuner_drv_shared:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/shared/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/shared/demod/none/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),drivers/shared/tuner/*.o);

clean_sttuner_mgr:
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/sat/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/cab/*.o);
	$(RM) $(subst $(BAD_SLASH),$(SLASHES),manager/ter/*.o);
endif

clean:
	$(MAKE) clean_sttuner_lib
	$(MAKE) clean_sttuner_drv
	$(MAKE) clean_sttuner_drv_sat
	$(MAKE) clean_sttuner_drv_cab
	$(MAKE) clean_sttuner_drv_ter
	$(MAKE) clean_sttuner_drv_shared
	$(MAKE) clean_sttuner_mgr


clean_sttuner:
	@echo Cleaning all in STTUNER tree...
	$(MAKE) clean
	$(MAKE) -C $(subst $(BAD_SLASH),$(SLASHES),./tests) clean

sttuner:
	@echo Making all in STTUNER tree...
	$(MAKE) sttuner.lib
	$(MAKE) -C $(subst $(BAD_SLASH),$(SLASHES),./tests) all

remake_sttuner:
	$(MAKE) clean_sttuner
	$(MAKE) sttuner


endif #corresponds to else of -> ifeq "$(DVD_OS)" "LINUX"      Initial line in this makefile

#EOF
