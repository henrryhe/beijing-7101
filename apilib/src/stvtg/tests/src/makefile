########################
# STVTG tests makefile #
########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stos stcommon stsys stapigat/src
COMMON_IMPORTS := stapigat stboot stos stcommon testtool stdenc stvtg
#stavmem
ST20_IMPORTS := $(COMMON_IMPORTS) sttbx stevt stpio stgxobj
ST200_IMPORTS := $(COMMON_IMPORTS) sttbx stevt stgxobj

TARGET_WITH_NO_STPWM = 5105 5107 7200 5162
ifneq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_NO_STPWM))" "$(DVD_FRONTEND)"
  ST20_IMPORTS += stpwm
  ST40_IMPORTS += stpwm
endif

LINUX_IMPORTS := stapigat stos stcommon testtool stdenc stvtg stevt stpio sttbx
ST40_IMPORTS := $(COMMON_IMPORTS) sttbx stevt stpio stgxobj


TARGET_WITH_STCFG = 5514 5516 5517
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STCFG))" "$(DVD_FRONTEND)"
  ST20_IMPORTS += stcfg
endif

TARGET_WITH_STI2C = mb282b mb314 mb361 mb382 mb295 espresso mb400 mb634

ifeq "$(findstring $(DVD_PLATFORM), $(TARGET_WITH_STI2C))" "$(DVD_PLATFORM)"
  ST20_IMPORTS += sti2c
endif

TARGET_WITH_STINTMR = 7015 7020 GX1
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STINTMR))" "$(DVD_BACKEND)"
  ST20_IMPORTS += stintmr
endif


TARGET_WITH_STAVMEM = 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STAVMEM))" "$(DVD_BACKEND)"
ST40_IMPORTS  += stavmem
LINUX_IMPORTS  += stavmem
endif

TARGET_WITH_STLAYER = 7100 7109
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STLAYER))" "$(DVD_BACKEND)"
ST40_IMPORTS  += stlayer
LINUX_IMPORTS  += stlayer
endif


TARGET_WITH_STCLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND),$(TARGET_WITH_STCLKRV))" "$(DVD_BACKEND)"
ST20_IMPORTS += stclkrv
ST40_IMPORTS += stclkrv
LINUX_IMPORTS += stclkrv
endif


ST20_OBJS := vtg_test.tco
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)

ifeq "$(DVD_OS)" "LINUX"
DVD_CFLAGS := $(DVD_CFLAGS) -DAVMEM_KERNEL_MODE
CFLAGS := $(CFLAGS) -DAVMEM_KERNEL_MODE
DVD_CFLAGS := $(DVD_CFLAGS)  -DSTTBX_PRINT
endif


ifneq ($(DEBUG),)
  STVTG_ICACHE_DISABLED := 1
  STVTG_DCACHE_DISABLED := 1
endif
ifeq ($(STVTG_DCACHE_DISABLED),)
  STAPIGAT_FLAGS +=  -DDCACHE_ENABLE
endif
ifeq ($(STVTG_ICACHE_DISABLED),)
  STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

ifeq "$(ARCHITECTURE)" "ST200"
	STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

# For output of toolbox
CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DSTTBX_INPUT -D STTBX_NO_UART
DVD_CFLAGS := $(DVD_CFLAGS) -D STTBX_NO_UART


BACKEND_WITHOUT_VTGIT = 5508 5510 5512 5514 5516 5517 5518
ifeq "$(findstring $(DVD_BACKEND), $(BACKEND_WITHOUT_VTGIT))" "$(DVD_BACKEND)"
  CFLAGS += -D STVTG_VSYNC_WITHOUT_VIDEO
  STAPIGAT_FLAGS += -DSTVTG_VSYNC_WITHOUT_VIDEO
  stvtg.lib: STVTG_VSYNC_WITHOUT_VIDEO=1
  export STVTG_VSYNC_WITHOUT_VIDEO
endif

BACKEND_USE_CLKRV = 7710 7100 7200
ifeq "$(findstring $(DVD_BACKEND), $(BACKEND_USE_CLKRV))" "$(DVD_BACKEND)"
  STAPIGAT_FLAGS += -DSTVTG_USE_CLKRV
  CFLAGS := $(CFLAGS) -D STVTG_USE_CLKRV
  export STVTG_USE_CLKRV
endif

ifneq "$(DVD_OS)" "LINUX"
BACKEND_CLKRV = 7109
ifeq "$(findstring $(DVD_BACKEND), $(BACKEND_CLKRV))" "$(DVD_BACKEND)"
  STAPIGAT_FLAGS += -DSTVTG_USE_CLKRV
  CFLAGS := $(CFLAGS) -D STVTG_USE_CLKRV
  export STVTG_USE_CLKRV
endif
endif

# Compile options
#----------------
ifneq ($(STVTG_WA_GNBVD35956),)
CFLAGS := $(CFLAGS) -D WA_GNBvd35956
endif

ifeq "$(DVD_BACKEND)" "7710"
  ifeq "$(STI7710_CUT2x)" "TRUE"
    CFLAGS := $(CFLAGS) -DSTI7710_CUT2x
  endif
endif

ifneq "$(STCLKRV_NO_PTI)" ""
     DVD_CFLAGS := $(DVD_CFLAGS) -D STCLKRV_NO_PTI
endif
ifeq "$(DVD_OS)" "LINUX"
  # STAVMEM is special in linux - no library is created so we must not add
  # the library to the link command. The following link option suppresses this.
  # This only works currently on LINUX.
  LINUX_libstavmem.a_NOLINK := 1
endif

stavmem.lib libstavmem.a: \
	STAVMEM_NO_GPDMA=1
	STAVMEM_NO_FDMA=1
export STAVMEM_NO_GPDMA
export STAVMEM_NO_FDMA

ifeq "$(ARCHITECTURE)" "ST20"
stapigat.lib:\
    DVD_CFLAGS = $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := stvtg/tests/src
    STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS) dumpreg
endif

libstapigat.a:\
    DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := stvtg/tests/src $(ST40_IMPORTS)
ifeq "$(DVD_OS)" "LINUX"
    STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS)
else
    STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS) dumpreg
endif



ifeq "$(ARCHITECTURE)" "ST200"
libstapigat.a:\
    DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := stvtg/tests/src
    STAPIGAT_IMPORTS_ST200 := $(ST200_IMPORTS) dumpreg
endif

export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40
export STAPIGAT_IMPORTS_ST200

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

# Target
ST20_TARGETS = vtg_test.lku
ST200_TARGETS := $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))


#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build general application for DRAM
vtg_test_INIT := board_init

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
$(ST20_TARGETS): $(IMPORT_LIBS) $(ST20_OBJS)
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
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS) $($(ARCHITECTURE)_TARGETS_OTHER)
ifeq "$(BUILDING_FOR)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(ARCHITECTURE)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(ARCHITECTURE)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample ST20 makefile




