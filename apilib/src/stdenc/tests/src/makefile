#########################
# STDENC tests makefile #
#########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stsys stapigat/src

COMMON_IMPORTS :=  stapigat stdenc stboot stos stcommon testtool

ST40_IMPORTS := $(COMMON_IMPORTS) sttbx stpio
ST20_IMPORTS := $(COMMON_IMPORTS) sttbx stpio
ST200_IMPORTS := $(COMMON_IMPORTS) sttbx

LINUX_IMPORTS :=  stapigat stdenc stcommon testtool sttbx

TARGET_WITH_NO_STPWM = 5105 5107 7200 5162
ifneq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_NO_STPWM))" "$(DVD_FRONTEND)"
  ST20_IMPORTS += stpwm
  ST40_IMPORTS += stpwm
endif


TARGET_WITH_STCFG = 5514 5516 5517
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STCFG))" "$(DVD_FRONTEND)"
  ST20_IMPORTS += stcfg
endif

TARGET_WITH_STI2C = mb282b mb314 mb361 mb382 mb295 espresso mb400
ifeq "$(findstring $(DVD_PLATFORM), $(TARGET_WITH_STI2C))" "$(DVD_PLATFORM)"
  ST20_IMPORTS += sti2c
endif

TARGET_WITH_STAVMEM = 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STAVMEM))" "$(DVD_BACKEND)"
  ST40_IMPORTS += stavmem stevt stgxobj
  LINUX_IMPORTS += stavmem stevt stos stgxobj

  # STAVMEM is special in linux - no library is created so we must not add
  # the library to the link command. The following link option suppresses this.
  # This only works currently on LINUX.
  LINUX_libstavmem.a_NOLINK := 1

stavmem.lib:\
        STAVMEM_NO_GPDMA=1
        STAVMEM_NO_FDMA=1
export STAVMEM_NO_GPDMA
export STAVMEM_NO_FDMA
endif

TARGET_WITH_STLAYER = 7100 7109
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STLAYER))" "$(DVD_BACKEND)"
  ST40_IMPORTS += stlayer
  LINUX_IMPORTS += stlayer
endif

NEED_DENC_MAIN_COMPENSATION = 5100 5105
ifeq "$(findstring $(DVD_BACKEND),$(NEED_DENC_MAIN_COMPENSATION))" "$(DVD_BACKEND)"
        DVD_CFLAGS += -DDENC_MAIN_COMPENSATION_ENABLED
        ifeq "$(findstring $(DENC_MAIN_COMPENSATION_VALUE),2)" "2"
               DVD_CFLAGS += -DDENC_MAIN_COMPONSATE_2PIXEL
        endif
        ifeq "$(findstring $(DENC_MAIN_COMPENSATION_VALUE),1.5)" "1.5"
               DVD_CFLAGS += -DDENC_MAIN_COMPONSATE_1_5PIXEL
        endif
        ifeq "$(findstring $(DENC_MAIN_COMPENSATION_VALUE),1)" "1"
               DVD_CFLAGS += -DDENC_MAIN_COMPONSATE_1PIXEL
        endif
        ifeq "$(findstring $(DENC_MAIN_COMPENSATION_VALUE),0.5)" "0.5"
               DVD_CFLAGS += -DDENC_MAIN_COMPONSATE_0_5PIXEL
        endif
endif
NEED_DENC_AUX_COMPENSATION = 5100
ifeq "$(findstring $(DVD_BACKEND),$(NEED_DENC_AUX_COMPENSATION))" "$(DVD_BACKEND)"
 #       DVD_CFLAGS += -DDENC_AUX_COMPENSATION_ENABLED
        ifeq "$(findstring $(DENC_AUX_COMPENSATION_VALUE),2)" "2"
               DVD_CFLAGS += -DDENC_AUX_COMPONSATE_2PIXEL
        endif
        ifeq "$(findstring $(DENC_AUX_COMPENSATION_VALUE),1.5)" "1.5"
               DVD_CFLAGS += -DDENC_AUX_COMPONSATE_1_5PIXEL
        endif
        ifeq "$(findstring $(DENC_AUX_COMPENSATION_VALUE),1)" "1"
               DVD_CFLAGS += -DDENC_AUX_COMPONSATE_1PIXEL
        endif
        ifeq "$(findstring $(DENC_AUX_COMPENSATION_VALUE),0.5)" "0.5"
               DVD_CFLAGS += -DDENC_AUX_COMPONSATE_0_5PIXEL
        endif
endif

# Object for ST20
ST20_OBJS := denc_test.tco
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

ifeq "$(DVD_OS)" "LINUX"
DVD_CFLAGS += -DSTTBX_PRINT
endif

# Object for ST200
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)

# For output of toolbox
CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT -D STTBX_NO_UART
DVD_CFLAGS := $(DVD_CFLAGS) -D STTBX_NO_UART

STAPIGAT_FLAGS :=
ifneq ($(DEBUG),)
  STDENC_ICACHE_DISABLED := 1
  STDENC_DCACHE_DISABLED := 1
endif
ifeq ($(STDENC_DCACHE_DISABLED),)
  STAPIGAT_FLAGS +=  -DDCACHE_ENABLE
endif
ifeq ($(STDENC_ICACHE_DISABLED),)
  STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

ifeq "$(BUILDING_FOR)" "ST200"
	STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

stapigat.lib libstapigat.a: \
    DVD_CFLAGS :=$(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := stdenc/tests/src $(ST40_IMPORTS)
    STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS) dumpreg
ifeq "$(DVD_OS)" "LINUX"
    STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS)
else
    STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS) dumpreg
endif
#temporary not enabling dumpreg for linux */
    STAPIGAT_IMPORTS_ST200 := $(ST200_IMPORTS)dumpreg

export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40
export STAPIGAT_IMPORTS_ST200

sti2c.lib: DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_MASTER_ONLY

# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS

# Target
ST20_TARGETS = denc_test.lku
ST200_TARGETS := $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))
ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))

LINUX_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

#Add Flag for map file
ifeq "$(BUILDING_FOR)" "ST20"
LKFLAGS= -M $(basename+ $@).map
endif

# Build general application for DRAM
denc_test_INIT := board_init

ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(IMPORT_LIBS) $(ST20_OBJS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

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

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(IMPORT_LIBS) $(ST40_OBJS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) $(BOARDSPECS)
endif


ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(IMPORT_LIBS) $(ST200_OBJS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(IMPORT_LIBS) $(LINUX_OBJS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

clean:
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)
ifeq "$(BUILDING_FOR)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(BUILDING_FOR)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample ST20 makefile


