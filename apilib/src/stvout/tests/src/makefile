#########################
# STVOUT tests makefile #
#########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stcommon stsys stapigat/src

# CAUTION: Imported library order is vital for linking aspect (stapigat must be very first imported library)
COMMON_IMPORTS := stapigat stboot stcommon stevt stgxobj testtool stdenc stvtg stvout stos

ifeq "$(STVOUT_CEC_CAPTURE)" "TRUE"
        DVD_CFLAGS :=$(DVD_CFLAGS)  -DSTVOUT_CEC_PIO_COMPARE -DSTVOUT_CEC_CAPTURE
else
ifeq "$(STVOUT_CEC_PIO_COMPARE)" "TRUE"
        DVD_CFLAGS :=$(DVD_CFLAGS)  -DSTVOUT_CEC_PIO_COMPARE
endif
endif

ifneq "$(DVD_OS)" "LINUX"
ifeq "$(STVOUT_UART_IO)" "TRUE"
        HEADER_IMPORTS += stuart
        COMMON_IMPORTS +=  stuart
        DVD_CFLAGS :=$(DVD_CFLAGS)  -DSTVOUT_TRACE -DSTTBX_FILTER -DUSE_UART_AS_STDIO
endif
ifeq "$(STVOUT_INTERRUPT_TRACE)" "TRUE"
        HEADER_IMPORTS += stuart
        COMMON_IMPORTS +=  stuart
        DVD_CFLAGS :=$(DVD_CFLAGS)  -D STVOUT_TRACE -D STVOUT_INTERRUPT_TRACE
endif
ifeq "$(STVOUT_STATE_MACHINE_TRACE)" "TRUE"
ifneq "$(STVOUT_INTERRUPT_TRACE)" "TRUE"
        HEADER_IMPORTS += stuart
        COMMON_IMPORTS +=  stuart
        DVD_CFLAGS :=$(DVD_CFLAGS)  -D STVOUT_TRACE
endif
        DVD_CFLAGS :=$(DVD_CFLAGS)  -D STVOUT_STATE_MACHINE_TRACE
endif
endif


ST20_IMPORTS := $(COMMON_IMPORTS) stpio sttbx
ST200_IMPORTS := $(COMMON_IMPORTS) stpio sttbx
ST40_IMPORTS := $(COMMON_IMPORTS) stpio sttbx
LINUX_IMPORTS := stapigat testtool stdenc stvtg stvout stos stcommon stevt stpio stpwm sttbx

TARGET_WITH_NO_STPWM = 5105 5107 7200 5162
ifneq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_NO_STPWM))" "$(DVD_FRONTEND)"
  ST20_IMPORTS += stpwm
  ST40_IMPORTS += stpwm
endif

TARGET_WITH_STCLKRV = 7710 7100 7200
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STCLKRV))" "$(DVD_FRONTEND)"
   ST20_IMPORTS += stclkrv
   ST40_IMPORTS += stclkrv
   LINUX_IMPORTS += stclkrv
endif

TARGET7109_WITH_STCLKRV = 7109
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET7109_WITH_STCLKRV))" "$(DVD_FRONTEND)"
   ST40_IMPORTS += stclkrv
   LINUX_IMPORTS += stclkrv
endif

TARGET_WITH_STCFG = 5514 5516 5517
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STCFG))" "$(DVD_FRONTEND)"
  ST20_IMPORTS += stcfg
endif


TARGET_WITH_STI2C = mb411 mb282b mb314 mb361 mb382 mb295 espresso mb376 mb390 mb391 mb400 mb457 mb428 mb519 mb634
ifeq "$(findstring $(DVD_PLATFORM), $(TARGET_WITH_STI2C))" "$(DVD_PLATFORM)"
  ST20_IMPORTS += sti2c
  ST40_IMPORTS += sti2c
  LINUX_IMPORTS += sti2c
  ST200_IMPORTS += sti2c
endif

TARGET_WITH_STINTMR = 7015 7020 GX1
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STINTMR))" "$(DVD_BACKEND)"
  ST20_IMPORTS += stintmr
endif

# Object for ST20
ST20_OBJS := vout_test.tco
# Object for ST200
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
# Object for ST40
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
# Object for LINUX
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)

ifeq "$(DVD_OS)" "LINUX"
DVD_CFLAGS := $(DVD_CFLAGS) -DAVMEM_KERNEL_MODE
CFLAGS := $(CFLAGS) -DAVMEM_KERNEL_MODE
DVD_CFLAGS := $(DVD_CFLAGS)  -DSTTBX_PRINT
endif

ifeq "$(DVD_BACKEND)" "7710"
  ifeq "$(STI7710_CUT2x)" "TRUE"
    CFLAGS := $(CFLAGS) -DSTI7710_CUT2x
  endif
  DVD_CFLAGS := $(DVD_CFLAGS) -DHDMI_I2C
endif

ifeq "$(STVOUT_HDCP_PROTECTION)" "TRUE"
     DVD_CFLAGS := $(DVD_CFLAGS) -DSTVOUT_HDCP_PROTECTION
endif

# For output of toolbox
ifeq "$(STVOUT_UART_IO)" "TRUE"
  CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT
else
  CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT -D STTBX_NO_UART
  DVD_CFLAGS := $(DVD_CFLAGS) -D STTBX_NO_UART
endif

STAPIGAT_FLAGS :=

#For Outputs dacs
TARGET_WITH_DENCV12 = 5528 5100 7710 5105 7100 5301 7109 5188 5525 7200
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_DENCV12))" "$(DVD_BACKEND)"
  STAPIGAT_FLAGS += -D STVOUT_DACS
endif
TARGET_WITH_DACS_ENHANCED = 5516
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_DACS_ENHANCED))" "$(DVD_BACKEND)"
  STAPIGAT_FLAGS += -D STVOUT_DACS_ENHANCED
endif
ifneq ($(DEBUG),)
  STVOUT_ICACHE_DISABLED := 1
  STVOUT_DCACHE_DISABLED := 1
endif
ifeq ($(STVOUT_DCACHE_DISABLED),)
  STAPIGAT_FLAGS +=  -DDCACHE_ENABLE
endif
ifeq ($(STVOUT_ICACHE_DISABLED),)
  STAPIGAT_FLAGS += -DICACHE_ENABLE
endif
ifeq "$(ARCHITECTURE)" "ST200"
	STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

TARGET_WITH_DVO = 5528 5516 5100 5301
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_DVO))" "$(DVD_FRONTEND)"
  STAPIGAT_FLAGS +=  -DDVO_TESTS
endif
#LINUX_IMPORTS += stavmem stlayer
TARGET_WITH_AVMEM_LAYER = 7100 7109
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_AVMEM_LAYER))" "$(DVD_FRONTEND)"
ST40_IMPORTS += stavmem stlayer
LINUX_IMPORTS += stavmem stlayer
stavmem.lib:\
	STAVMEM_NO_GPDMA=1
	STAVMEM_NO_FDMA=1
export STAVMEM_NO_GPDMA
export STAVMEM_NO_FDMA
endif

TARGET_WITH_AVMEM = 7200
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_AVMEM))" "$(DVD_FRONTEND)"
ST40_IMPORTS += stavmem
LINUX_IMPORTS += stavmem
stavmem.lib:\
	STAVMEM_NO_GPDMA=1
	STAVMEM_NO_FDMA=1
export STAVMEM_NO_GPDMA
export STAVMEM_NO_FDMA
endif

ifeq "$(DVD_OS)" "LINUX"
  # STAVMEM is special in linux - no library is created so we must not add
  # the library to the link command. The following link option suppresses this.
  # This only works currently on LINUX.
  LINUX_libstavmem.a_NOLINK := 1
endif

ifeq "$(ARCHITECTURE)" "ST20"
stapigat.lib:\
    DVD_CFLAGS = $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := stvout/tests/src
    STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS)
endif

libstapigat.a:\
    DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := stvout/tests/src $(ST40_IMPORTS)
ifeq "$(DVD_OS)" "LINUX"
    STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS)
else
    STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS)
endif

ifeq "$(ARCHITECTURE)" "ST200"
libstapigat.a:\
    DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := stvout/tests/src
    STAPIGAT_IMPORTS_ST200 := $(ST200_IMPORTS)
endif



export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40
export STAPIGAT_IMPORTS_ST200


sti2c.lib: DVD_CFLAGS = $(DVD_CFLAGS) -DSTI2C_MASTER_ONLY

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

ifneq ($(DEBUG),)
  STVOUT_ICACHE_DISABLED := 1
  STVOUT_DCACHE_DISABLED := 1
endif
ifeq ($(STVOUT_DCACHE_DISABLED),)
  CFLAGS := $(CFLAGS) -D DCACHE_ENABLE
endif
ifeq ($(STVOUT_ICACHE_DISABLED),)
  CFLAGS := $(CFLAGS) -D ICACHE_ENABLE
endif


# Target
ST20_TARGETS = vout_test.lku
ST20_TARGETS_OTHER = vout_test.dbg

ST200_TARGETS = $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))
ST200_TARGETS_OTHER =

ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
LINUX_TARGETS = $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build general application for DRAM
vout_test_INIT := board_init

BOARDSPECS=
ifeq "$(ARCHITECTURE)" "ST40"
	ifeq "$(DVD_BACKEND)" "7200"
		ifndef MULTICOM_COMPANION_CONFIG
			export MULTICOM_COMPANION_CONFIG=0x07
		endif
            export USE_MBOARD=board_$(DVD_PLATFORM)_lmi0_$(MULTICOM_COMPANION_CONFIG)
            BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7200
	endif
endif


ifeq "$(BUILDING_FOR)" "ST20"
	LKFLAGS = -M $(basename $@).map

$(ST20_TARGETS): $(ST20_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) $(BOARDSPECS)
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(IMPORT_LIBS) $(LINUX_OBJS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

clean:
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS) $($(BUILDING_FOR)_TARGETS_OTHER)
ifeq "$(BUILDING_FOR)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(BUILDING_FOR)_TARGETS))
	$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample makefile

