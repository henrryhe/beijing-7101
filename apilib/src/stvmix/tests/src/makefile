#########################
# STVMIX tests makefile #
#########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stcommon stsys stapigat/src/patch stapigat/src
ifeq "$(DVD_OS)" "LINUX"
HEADER_IMPORTS += stapigat/src/linux
endif

# CAUTION: Imported library order is vital for linking aspect (stapigat must be very first imported library)
COMMON_IMPORTS := stapigat stcommon stos stboot testtool stvtg stvout
COMMON_IMPORTS += stdenc stgxobj stlayer stvmix stavmem sttbx stevt

ST20_IMPORTS := $(COMMON_IMPORTS) stpio


ST200_IMPORTS := $(COMMON_IMPORTS)


LINUX_IMPORTS := stapigat stcommon stos testtool stdenc stvtg stvout stvmix stlayer stevt stavmem stpio

ifneq "$(DVD_BACKEND)" "7200"
	ST40_IMPORTS := $(COMMON_IMPORTS) stpio stpwm
else
	ST40_IMPORTS := $(COMMON_IMPORTS) stpio
endif

ifneq (,$(findstring $(DVD_BACKEND),5100 5105 5188 5107 5162))
        ST20_IMPORTS += stdisp compo sti2c
        STAPIGAT_FLAGS := -DSTVMIX_USE_GLOBAL_DEVICE_TYPE
endif

ifneq (,$(findstring $(DVD_BACKEND),5301 5525))
        ST200_IMPORTS := $(ST200_IMPORTS) stdisp compo  stpio sti2c
        STAPIGAT_FLAGS := -DSTVMIX_USE_GLOBAL_DEVICE_TYPE
endif

ifeq "$(STLAYER_CONVERT_PALETTE_TO_YCBCR888)" "TRUE"
        STAPIGAT_FLAGS += -DSTLAYER_CONVERT_PALETTE_TO_YCBCR888
endif

ifeq "$(STLAYER_CONVERT_PALETTE_TO_AYCBCR8888)" "TRUE"
        STAPIGAT_FLAGS += -DSTLAYER_CONVERT_PALETTE_TO_AYCBCR8888
endif


ifneq (,$(findstring $(DVD_BACKEND),5105))
ifeq "$(SELECT_DEVICE_STB5118)" "TRUE"
        STAPIGAT_FLAGS += -DSELECT_DEVICE_STB5118
endif
endif



ifeq "$(USE_215K_BLITTER_CACHE)" "TRUE"
    STAPIGAT_FLAGS += -DUSE_215K_BLITTER_CACHE
else
        ifeq "$(USE_128K_BLITTER_CACHE)" "TRUE"
                STAPIGAT_FLAGS += -DUSE_128K_BLITTER_CACHE
        else
                ifeq "$(USE_64K_BLITTER_CACHE)" "TRUE"
                        STAPIGAT_FLAGS += -DUSE_64K_BLITTER_CACHE
                else
                        ifeq "$(USE_32K_BLITTER_CACHE)" "TRUE"
                                STAPIGAT_FLAGS += -DUSE_32K_BLITTER_CACHE
                        endif
                endif
        endif
endif


ifneq (,$(findstring $(DVD_BACKEND),5301 5525))
        ifeq "$(STVMIX_USE_UART)" "TRUE"
                CFLAGS := $(CFLAGS) -D STVMIX_USE_UART -D STTBX_FILTER
                ST200_IMPORTS := $(ST200_IMPORTS)  stuart
              DVD_CFLAGS :=$(DVD_CFLAGS)  -D STTBX_FILTER -D STVMIX_USE_UART

        else
                CFLAGS := $(CFLAGS) -D STTBX_NO_UART
              DVD_CFLAGS :=$(DVD_CFLAGS)  -D STTBX_NO_UART
        endif
else
                CFLAGS := $(CFLAGS) -D STTBX_NO_UART
              DVD_CFLAGS :=$(DVD_CFLAGS)  -D STTBX_NO_UART
endif





ifneq (,$(findstring $(DVD_BACKEND), 5100))
    #Chroma Luma Delay WA is enabled only if WA_CLDELAY_USED is defined
    ifeq "$(WA_CLDELAY_USED)" "TRUE"
        DVD_CFLAGS += -DWA_GNBvd31390
    endif
endif


TARGET_WITH_STCFG = 5514 5516 5517
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STCFG))" "$(DVD_FRONTEND)"
  ST20_IMPORTS += stcfg
endif

TARGET_WITH_STI2C = mb282b mb314 mb361 mb295 mb382 espresso mb391 mb411 mb519
ifeq "$(findstring $(DVD_PLATFORM), $(TARGET_WITH_STI2C))" "$(DVD_PLATFORM)"
  ST20_IMPORTS += sti2c
  ST40_IMPORTS += sti2c
  LINUX_IMPORTS += sti2c
endif

TARGET_WITH_STCLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND),$(TARGET_WITH_STCLKRV))" "$(DVD_BACKEND)"
ST20_IMPORTS += stclkrv
ST40_IMPORTS += stclkrv
LINUX_IMPORTS += stclkrv
endif

TARGET_WITH_STINTMR = 7015 7020 GX1
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STINTMR))" "$(DVD_BACKEND)"
  ST20_IMPORTS += stintmr
endif


SPECIFIC_VTG_CFLAGS :=
TARGET_WITH_VSYNC_WITHOUT_VIDEO = 5510 5512 5518 5508 5514 5516 5517
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_VSYNC_WITHOUT_VIDEO))" "$(DVD_BACKEND)"
  SPECIFIC_VTG_CFLAGS := -DSTVTG_VSYNC_WITHOUT_VIDEO
  STVTG_VSYNC_WITHOUT_VIDEO := 1
  export STVTG_VSYNC_WITHOUT_VIDEO
endif

ST20_OBJS := vmix_test.tco


ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


ifeq "$(DVD_OS)" "LINUX"
DVD_CFLAGS := $(DVD_CFLAGS)  -DSTTBX_PRINT
endif

# Flags for all drivers
CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS) $(SPECIFIC_VTG_CFLAGS)

# I & D cache options
ifneq ($(DEBUG),)
  STVMIX_ICACHE_DISABLED := 1
  STVMIX_DCACHE_DISABLED := 1
endif
ifeq ($(STVMIX_DCACHE_DISABLED),)
  STAPIGAT_FLAGS := $(STAPIGAT_FLAGS) -DDCACHE_ENABLE
endif
ifeq ($(STVMIX_ICACHE_DISABLED),)
  STAPIGAT_FLAGS := $(STAPIGAT_FLAGS) -DICACHE_ENABLE
endif

ifeq "$(DVD_BACKEND)" "7710"
  ifeq "$(STI7710_CUT2x)" "TRUE"
     CFLAGS := $(CFLAGS) -DSTI7710_CUT2x
  endif
endif


ifeq "$(DVD_BACKEND)" "7109"
  ifeq "$(STI7109_CUT2)" "TRUE"
     CFLAGS := $(CFLAGS) -DSTI7109_CUT2
     DVD_CFLAGS := $(DVD_CFLAGS) -DSTI7109_CUT2
  endif
endif



ifeq "$(COMPO_SIMULATOR)" "TRUE"
    COMPO_FLAGS += -DPATCH_SIMULATOR
    CFLAGS := $(CFLAGS) -DPATCH_SIMULATOR
endif

# For output of toolbox
CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DSTTBX_INPUT -D STTBX_REPORT
ifeq ($(DEBUG), 1)
	CFLAGS := $(CFLAGS) -O0 -DDEBUG
endif

DVD_CFLAGS := $(DVD_CFLAGS) $(SPECIFIC_VTG_CFLAGS)

ifeq "$(COMPO_NO_STATIC_BASE_ADDRESS)" "TRUE"
    COMPO_FLAGS += -DDONT_USE_STATIC_BASE_ADDRESS
    CFLAGS := $(CFLAGS) -DDONT_USE_STATIC_BASE_ADDRESS
endif

ifeq "$(ARCHITECTURE)" "ST200"
	STAPIGAT_FLAGS += -DICACHE_ENABLE
#      STAPIGAT_FLAGS +=  -DDCACHE_ENABLE
endif

TARGET_WITH_STCLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND),$(TARGET_WITH_STCLKRV))" "$(DVD_BACKEND)"
  STAPIGAT_FLAGS := -DSTVOUT_DACS
endif


compo.lib : DVD_CFLAGS := $(DVD_CFLAGS) $(COMPO_FLAGS)

stdisp.lib : DVD_CFLAGS :=$(DVD_CFLAGS) $(COMPO_FLAGS)

stlayer.lib libstlayer.a:\
    DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_GAMMAGFX_TASK_PRIORITY=7

stvtg.lib libstvtg.a:\
   DVD_CFLAGS := $(DVD_CFLAGS) -DSTVTG_TASK_PRIORITY_IT_PROCESSING=15

ifneq (,$(findstring $(DVD_BACKEND),5301 5525))
sttbx.lib libsttbx.a: DVD_CFLAGS :=$(DVD_CFLAGS)  -DSTTBX_PRINT -D STTBX_INPUT
endif

ifneq "$(STCLKRV_NO_PTI)" ""
     DVD_CFLAGS:= $(DVD_CFLAGS) -DSTCLKRV_NO_PTI
endif

stapigat.lib libstapigat.a:\
   DVD_CFLAGS := $(DVD_CFLAGS)  -DSTTBX_PRINT -D STTBX_INPUT $(STAPIGAT_FLAGS)
   STAPIGAT_INCLUDE := stvmix/tests/src $(ST40_IMPORTS)

ifeq "$(DVD_OS)" "LINUX"
   STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS)
else
  STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS) dumpreg
endif
   STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS) dumpreg
   STAPIGAT_IMPORTS_ST200 := $(ST200_IMPORTS) dumpreg


stvmix.lib libstvmix.a:\
   DVD_CFLAGS := $(DVD_CFLAGS)


export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40
export STAPIGAT_IMPORTS_ST200

 # STAVMEM is special in linux - no library is created so we must not add
 # the library to the link command. The following link option suppresses this.
 # This only works currently on LINUX.

LINUX_libstavmem.a_NOLINK := 1

stavmem.lib libstavmem.a:\
	STAVMEM_NO_GPDMA=1
	STAVMEM_NO_FDMA=1
export STAVMEM_NO_GPDMA
export STAVMEM_NO_FDMA

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

MESSAGE1=.
ifeq "$(DVD_FRONTEND)" "5518"
	MESSAGE1=Compilation with DVD_FRONTEND_CPUCLOCK=$(DVD_FRONTEND_CPUCLOCK)
	ifneq ($(DVD_FRONTEND_CPUCLOCK),81)
		MESSAGE1=!!! Warning : DVD_FRONTEND_CPUCLOCK should be 81 !!!
	endif
endif

# Target
ST20_TARGETS = vmix_test.lku
ST20_TARGETS_OTHER = vmix_test.dbg
ST200_TARGETS = $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))
ST200_TARGETS_OTHER =

LINUX_TARGETS = $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))
ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST40_TARGETS_OTHER =


#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build general application for DRAM
vmix_test_INIT := board_init

ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(IMPORT_LIBS) $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

BOARDSPECS=
ifeq "$(ARCHITECTURE)" "ST40"
	ifeq "$(DVD_BACKEND)" "7200"
      export USE_MBOARD=board_$(DVD_PLATFORM)_lmi0_$(MULTICOM_COMPANION_CONFIG)
	BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7200
	endif
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(IMPORT_LIBS) $(ST40_OBJS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) $(BOARDSPECS)
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(IMPORT_LIBS) $(SUBDIR_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS) $(IMPORT_LIBS) $(SUBDIR_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS) $($(BUILDING_FOR)_TARGETS_OTHER)
ifeq "$(BUILDING_FOR)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(BUILDING_FOR)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample ST20 makefile













