##########################
# STLAYER tests makefile #
##########################
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stos stcommon stsys stapigat/src stapigat/src/patch stavmem
ifeq "$(DVD_OS)" "LINUX"
HEADER_IMPORTS += stapigat/src/linux
endif

ifneq "$(DVD_OS)" "LINUX"
# Imported composants
COMMON_IMPORTS  := stboot stos stcommon stevt testtool  stvtg stvout stdenc stgxobj
COMMON_IMPORTS  += stlayer stavmem sttbx stapigat stvmix sti2c stpio

IMPORTS := $(COMMON_IMPORTS)
endif

ifneq (,$(findstring $(DVD_BACKEND),5105 5188 5107 5162))
    IMPORTS := $(IMPORTS) stcfg
endif

ifneq (,$(findstring $(DVD_BACKEND),5301 5525))
        ifeq "$(STLAYER_USE_UART)" "TRUE"
                CFLAGS := $(CFLAGS) -D STLAYER_USE_UART -D STTBX_FILTER
                COMMON_IMPORTS := $(COMMON_IMPORTS) stuart
                DVD_CFLAGS :=$(DVD_CFLAGS)  -D STTBX_FILTER -D STLAYER_USE_UART

        else
                CFLAGS := $(CFLAGS) -D STTBX_NO_UART
                DVD_CFLAGS :=$(DVD_CFLAGS)  -D STTBX_NO_UART
        endif
else
                CFLAGS := $(CFLAGS) -D STTBX_NO_UART
                DVD_CFLAGS :=$(DVD_CFLAGS)  -D STTBX_NO_UART
endif



ifeq "$(DVD_OS)" "LINUX"

IMPORTS := stapigat stos stcommon testtool stvmix
IMPORTS += stdenc stvout stvtg stlayer stevt stavmem sti2c stpio



#IMPORTS := $(LINUX_IMPORTS)
DVD_CFLAGS := $(DVD_CFLAGS) -DSTTBX_PRINT -DSTLINUX_RT_THREADS
endif

# stgxobj stevt stintmr stpwm


# Object for
ST20_OBJS :=  lay_test.tco tt_gutil.tco lay_cmd.tco
ifeq "$(DVD_BACKEND)" "7109"
        ifneq ($(DVD_SECURED_CHIP),)
                ST20_OBJS :=  $(ST20_OBJS) lay_secure.tco
        endif
	ifneq ($(STLAYER_NO_STMES),)
		DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_NO_STMES
		CFLAGS := $(CFLAGS) -D STLAYER_NO_STMES
	endif
endif


ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))




STAPIGAT_FLAGS :=
ifneq (,$(findstring $(DVD_BACKEND),7015 7020))
    IMPORTS += stintmr
endif

ifneq (,$(findstring $(DVD_FRONTEND),5100 5105 5188 5107 5162))
    IMPORTS += stdisp compo stfdma

endif

ifneq (,$(findstring $(DVD_FRONTEND),5514))
    IMPORTS += stgpdma
endif
ifneq (,$(findstring $(DVD_FRONTEND),5528))
    IMPORTS += stgpdma
endif

ifneq (,$(findstring $(DVD_FRONTEND),7710))
    IMPORTS += stfdma
    ST40_IMPORTS += stfdma
    ST40_IMPORTS += stclkrv
endif

ifneq "$(DVD_OS)" "LINUX"
ifneq (,$(findstring $(DVD_FRONTEND),7710 7100 7109 7200))
    IMPORTS += stclkrv
    ST40_IMPORTS += stclkrv
endif
endif





ifneq (,$(findstring $(DVD_FRONTEND),5517))
    IMPORTS += stfdma
endif

ifneq (,$(findstring $(DVD_BACKEND),5514 ))
    IMPORTS +=  stgpdma
    DVD_CFLAGS:= $(DVD_CFLAGS) -D STVTG_VSYNC_WITHOUT_VIDEO
    CFLAGS := $(CFLAGS) -D STVTG_VSYNC_WITHOUT_VIDEO
    STAPIGAT_FLAGS += -D STVTG_VSYNC_WITHOUT_VIDEO
endif

ifneq (,$(findstring $(DVD_BACKEND),5516 ))
    IMPORTS +=
    stavmem.lib: STAVMEM_NO_2DBM=1
    export STAVMEM_NO_2DBM
    DVD_CFLAGS:= $(DVD_CFLAGS) -D STVTG_VSYNC_WITHOUT_VIDEO
    CFLAGS := $(CFLAGS) -D STVTG_VSYNC_WITHOUT_VIDEO
    STAPIGAT_FLAGS += -D STVTG_VSYNC_WITHOUT_VIDEO
endif

ifneq (,$(findstring $(DVD_BACKEND),5517 ))
    IMPORTS += stfdma
    stavmem.lib: STAVMEM_NO_2DBM=1
    export STAVMEM_NO_2DBM
    DVD_CFLAGS:= $(DVD_CFLAGS) -D STVTG_VSYNC_WITHOUT_VIDEO
    CFLAGS := $(CFLAGS) -D STVTG_VSYNC_WITHOUT_VIDEO
    STAPIGAT_FLAGS += -D STVTG_VSYNC_WITHOUT_VIDEO
endif

ifeq "$(BUILDING_FOR)" "ST200"
        STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

ifneq (,$(findstring $(DVD_BACKEND),GX1 ))
    ST40_IMPORTS += stintmr
	ST40_OBJS    += clintmr.o
endif

# STBLIT Test
ifeq "$(ENABLE_STBLIT_MACROS)" "TRUE"
    IMPORTS += stblit
endif

ifeq "$(DVD_BACKEND)" "7109"
  ifeq "$(STI7109_CUT2)" "TRUE"
     CFLAGS := $(CFLAGS) -D STI7109_CUT2
     DVD_CFLAGS := $(DVD_CFLAGS) -D STI7109_CUT2
     STAPIGAT_FLAGS += -D STI7109_CUT2
  endif
endif

# For output of toolbox
CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DSTTBX_INPUT -D STTBX_REPORT

ifeq "$(DVD_BACKEND)" "7109"
        ifneq ($(DVD_SECURED_CHIP),)
                CFLAGS := $(CFLAGS) -D DVD_SECURED_CHIP
		ifeq ($(STLAYER_NO_STMES),)
                FULLDIR := $(DVD_ROOT)/dvdgr-prj-stavmem
                LDOPTS += $(FULLDIR)/libstmes.a
	endif
        endif
endif


ST40_IMPORTS := $(IMPORTS)

ST200_IMPORTS := $(COMMON_IMPORTS)

LINUX_IMPORTS := $(IMPORTS) stclkrv 

ifneq (,$(findstring $(DVD_FRONTEND),5301 5525))
    ST200_IMPORTS := $(ST200_IMPORTS) stdisp compo stfdma
endif


# Target
ST20_TARGETS        = lay_test.lku
ST20_TARGETS_OTHER  = lay_test.dbg

ST40_TARGETS        = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST40_TARGETS_OTHER  =

ST200_TARGETS        = $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))
ST200_TARGETS_OTHER  =

LINUX_TARGETS        = $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))
LINUX_TARGETS_OTHER  =

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

#Compil flags
# STBLIT Test
ifeq "$(ENABLE_STBLIT_MACROS)" "TRUE"
  STAPIGAT_FLAGS  += -D ENABLE_STBLIT_MACROS
endif

ifeq "$(STLAYER_CONVERT_PALETTE_TO_YCBCR888)" "TRUE"
    STAPIGAT_FLAGS  += -D STLAYER_CONVERT_PALETTE_TO_YCBCR888
endif


CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT -D STTBX_REPORT -D STTBX_NO_UART
ifneq ($(DEBUG),)
    STVID_ICACHE_DISABLED := 1
    STVID_DCACHE_DISABLED := 1
endif

ifeq ($(STVID_DCACHE_DISABLED),)
    STAPIGAT_FLAGS += -D DCACHE_ENABLE
endif
ifeq ($(STVID_ICACHE_DISABLED),)
    STAPIGAT_FLAGS += -D ICACHE_ENABLE
endif

# Specific mb290 option
ifeq "$(DVD_PLATFORM)" "mb290"
    STCOMMON_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
    STPTI_OPTIONS += -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
endif
# Following options are temporary !!!!
# Remove DDR test for STi7020 cut 1.0
ifeq "$(DVD_BACKEND)" "7020"
    STBOOT_OPTIONS += -DSTBOOT_DISABLE_RAM_TESTS
endif
#end of temporary !!!

ifneq (,$(findstring $(DVD_FRONTEND),5100 5105 5301 5188 5525 5107 5162))
ifeq "$(COMPO_NO_STATIC_BASE_ADDRESS)" "TRUE"
    COMPO_FLAGS += -DDONT_USE_STATIC_BASE_ADDRESS
    CFLAGS := $(CFLAGS) -DDONT_USE_STATIC_BASE_ADDRESS
endif
compo.lib libcompo.a: DVD_CFLAGS +=  $(COMPO_FLAGS)
endif

  # STAVMEM is special in linux - no library is created so we must not add
  # the library to the link command. The following link option suppresses this.
  # This only works currently on LINUX.
  LINUX_libstavmem.a_NOLINK := 1

stavmem.lib: \
	STAVMEM_NO_GPDMA=1
	STAVMEM_NO_FDMA=1
export STAVMEM_NO_GPDMA
export STAVMEM_NO_FDMA

#stboot.lib libstboot.a:\
stboot.lib :\
stclkrv.lib :\
        DVD_CFLAGS += $(STCLKRV_OPTIONS)

#stcommon.lib libstcommon.a:\
stcommon.lib :\
        DVD_CFLAGS = $(DVD_CFLAGS) $(STCOMMON_OPTIONS)

#sttbx.lib libsttbx.a: \
#sttbx.lib : \
#        DVD_CFLAGS = $(DVD_CFLAGS) -DSTTBX_NO_UART
#stuart.lib libstuart.a: \
stuart.lib libstuart.a :\
        DVD_CFLAGS = $(DVD_CFLAGS) -DSTUART_TIMER_TASK_PRIORITY=3
#sttbx.lib libsttbx.a:\
sttbx.lib :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTTBX_NO_UART
#stboot.lib libstboot.a:\
stboot.lib libstboot.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTBOOT_DISABLE_RAM_TESTS
export DVD_CFLAGS
#stapigat.lib libstapigat.a:\
#stapigat.lib:\
#   DVD_CFLAGS =$(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT -DSTTBX_NO_UART -DSTCLKRV_NO_PTI
stapigat.lib : DVD_CFLAGS += $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT  -DSTCLKRV_NO_PTI
libstapigat.a : DVD_CFLAGS +=$(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT  -DSTCLKRV_NO_PTI
    STAPIGAT_INCLUDE := stlayer/tests/src $(IMPORTS)
    STAPIGAT_IMPORTS_ST20 := $(IMPORTS)
ifeq "$(DVD_OS)" "LINUX"
   STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS)
else
    STAPIGAT_IMPORTS_ST40 := $(IMPORTS)
endif

    STAPIGAT_IMPORTS_ST200 := $(ST200_IMPORTS)


export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40
export STAPIGAT_IMPORTS_ST200

ifeq ($(FLOATING_POINT), fp)
        LKFLAGS += -fp
endif

# Build general application for DRAM
lay_test_INIT := board_init
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(IMPORT_LIBS) $(ST20_OBJS)
	$(ECHO) Linking $@
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
$(ST200_TARGETS):  $(IMPORT_LIBS) $(ST200_OBJS)
	$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif


ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS):  $(IMPORT_LIBS) $(LINUX_OBJS)
	$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif


clean: $(SUBDIR_CLEAN)
	$(ECHO) Cleaning $(TARGETS)
	$(RM) $($(BUILDING_FOR)_OBJS)
	$(RM) $($(BUILDING_FOR)_TARGETS) $($(BUILDING_FOR)_TARGETS_OTHER)

ifeq "$(BUILDING_FOR)" "ST20"
	$(RM) $(subst .lku,.dbg,$($(BUILDING_FOR)_TARGETS))
	$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif
