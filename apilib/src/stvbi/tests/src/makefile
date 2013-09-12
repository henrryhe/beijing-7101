########################
# STVBI tests makefile #
########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stos stcommon stsys stapigat/src stapigat/src/patch
ifeq "$(DVD_OS)" "LINUX"
HEADER_IMPORTS += stapigat/src/linux
endif

COMMON_IMPORTS := stapigat stboot stcommon stos testtool stvtg stvout stvbi stdenc stevt
COMMON_IMPORTS += stavmem

COMMON_IMPORTS += sttbx stgxobj

ST40_IMPORTS := $(COMMON_IMPORTS)

TARGET_WITH_NO_STPWM = 5105 5107 7200
ifneq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_NO_STPWM))" "$(DVD_FRONTEND)"
  ST20_IMPORTS += stpwm
  ST40_IMPORTS += stpwm
endif

TARGET_WITH_STMERGE = 5100
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STMERGE))" "$(DVD_FRONTEND)"
       COMMON_IMPORTS += stmerge
endif

TARGET_WITH_STLAYER = 7100 7109 7710 5525 5105 5188 5107 7200
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STLAYER))" "$(DVD_BACKEND)"
  COMMON_IMPORTS += stlayer
endif

TARGET_WITH_STVMIX = 7710 5525 5105 7100 7109 5188 5107 7200
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STVMIX))" "$(DVD_BACKEND)"
  COMMON_IMPORTS += stvmix
endif

LINUX_IMPORTS := stapigat stpio stos stcommon testtool stvtg stvout stvbi stdenc stevt stlayer stvmix stavmem


TARGET_WITH_STCFG = 5514 5516 5517
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STCFG))" "$(DVD_FRONTEND)"
  COMMON_IMPORTS += stcfg
endif

TARGET_WITH_STI2C = mb282b mb314 mb361 mb382 mb295 mb411 mb391 mb519
ifeq "$(findstring $(DVD_PLATFORM), $(TARGET_WITH_STI2C))" "$(DVD_PLATFORM)"
  COMMON_IMPORTS += sti2c
endif

TARGET_WITH_STINTMR = 7015 7020 GX1
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STINTMR))" "$(DVD_BACKEND)"
  COMMON_IMPORTS += stintmr
endif

TARGET_WITH_STCLKRV = 7710 7100 5105 5525 7109 5188 5107 5100 7200
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STCLKRV))" "$(DVD_BACKEND)"
  COMMON_IMPORTS += stclkrv
  ST40_IMPORTS += stclkrv
  ST200_IMPORTS += stclkrv
  LINUX_IMPORTS += stclkrv
endif


TARGET_WITH_DISP = 5105 5525 5107 5188
ifeq "$(findstring $(DVD_BACKEND),$(TARGET_WITH_DISP))" "$(DVD_BACKEND)"
  ST20_IMPORTS := $(COMMON_IMPORTS) stuart stpio  compo stdisp
  COMMON_IMPORTS :=  $(COMMON_IMPORTS) compo stdisp
  ST200_IMPORTS :=  $(ST200_IMPORTS) compo stdisp
endif

ifneq "$(DVD_OS)" "LINUX"
TARGET_WITHOUT_PTI =  7100 7109 5301 5525 5188 5107 5105 7200
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITHOUT_PTI))" "$(DVD_BACKEND)"
  ST20_IMPORTS := $(COMMON_IMPORTS) stuart stpio 
  ST40_IMPORTS := $(COMMON_IMPORTS) stuart stpio 
  ST200_IMPORTS := $(COMMON_IMPORTS) stuart stpio
else
  ifneq (,$(findstring $(DVD_FRONTEND),7710 5107))
        COMMON_IMPORTS += $(DVD_TRANSPORT)lt
  else
         COMMON_IMPORTS += $(DVD_TRANSPORT)
  endif
  ST20_IMPORTS := $(COMMON_IMPORTS) stuart stpio 
  ST40_IMPORTS := $(COMMON_IMPORTS) stuart stpio 
  ST200_IMPORTS :=  $(COMMON_IMPORTS) stuart stpio
endif
endif

ifneq (,$(findstring $(DVD_BACKEND),5301 5525))
        ifeq "$(STVBI_USE_UART)" "TRUE"
                CFLAGS := $(CFLAGS) -D STVMIX_USE_UART -D STTBX_FILTER
                DVD_CFLAGS :=$(DVD_CFLAGS)  -D STTBX_FILTER -D STVMIX_USE_UART

        else
                CFLAGS := $(CFLAGS) -D STTBX_NO_UART
              DVD_CFLAGS :=$(DVD_CFLAGS)  -D STTBX_NO_UART
        endif
else
                CFLAGS := $(CFLAGS) -D STTBX_NO_UART
              DVD_CFLAGS :=$(DVD_CFLAGS)  -D STTBX_NO_UART
endif



# no teletext tests on 7015/7020 plugged on 5517
TARGET_WITHOUT_TTX_TEST = 7015 7020 5528 5105 7100 7109 5301 5525 5188 5107 7200
TARGET_WITHOUT_TTX_TEST_FE = 5517
ifneq "$(findstring $(DVD_BACKEND), $(TARGET_WITHOUT_TTX_TEST))" "$(DVD_BACKEND)"
  ifneq "$(findstring $(DVD_FRONTEND), $(TARGET_WITHOUT_TTX_TEST_FE))" "$(DVD_FRONTEND)"
        COMMON_IMPORTS += stttx
        ST20_IMPORTS := $(ST20_IMPORTS) stttx
  endif
endif

ifeq "$(DVD_BACKEND)" "7710"
  ifeq "$(STI7710_CUT2x)" "TRUE"
    CFLAGS := $(CFLAGS) -DSTI7710_CUT2x
  endif
endif


#Teletext test on 5517
TARGET_WITH_TTX = 5517
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_TTX))" "$(DVD_BACKEND)"
        COMMON_IMPORTS += stttx
        ST20_IMPORTS := $(ST20_IMPORTS) stttx
endif

#Teletext test on 7020
TARGET_WITH_TTX_TEST = TP3
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_TTX_TEST))" "$(DVD_FRONTEND)"
        COMMON_IMPORTS += stttx
        ST20_IMPORTS := $(ST20_IMPORTS) stttx
endif

# Object for ST20
ST20_OBJS := vbi_test.tco

# Object for ST40


LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

ifneq "$(STCLKRV_NO_PTI)" ""
      DVD_CFLAGS:= $(DVD_CFLAGS) -DSTCLKRV_NO_PTI
endif

DVD_CFLAGS := $(DVD_CFLAGS) -DSTTBX_INPUT

# Object for ST200
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

#Activating MultiHandle Closed Caption
ifeq ($(VBI_MULT_CC), 1)
     DVD_CFLAGS+= -DVBI_MULT_CC
endif


# For compilation of pti+link or stpti
CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)

# I & D cache options
ifneq ($(DEBUG),)
  STVBI_ICACHE_DISABLED := 1
  STVBI_DCACHE_DISABLED := 1
endif
ifeq ($(STVBI_DCACHE_DISABLED),)
  STAPIGAT_FLAGS := $(STAPIGAT_FLAGS) -DDCACHE_ENABLE
endif
ifeq ($(STVBI_ICACHE_DISABLED),)
  STAPIGAT_FLAGS := $(STAPIGAT_FLAGS) -DICACHE_ENABLE
endif
# ICache must be enabled for st200
ifeq "$(BUILDING_FOR)" "ST200"
	STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

# For output of toolbox
CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DSTTBX_INPUT

#For Outputs dacs
TARGET_WITH_DENCV12 = 5528 5100 5105 5301 7710 7100 7109 5525 7200
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_DENCV12))" "$(DVD_BACKEND)"
  STAPIGAT_FLAGS += -DSTVOUT_DACS
endif

ifeq "$(DVD_TRANSPORT)" "stpti4"
   ifeq ($(DVD_SERVICE),DVB)
       STAPIGAT_FLAGS += -DSTPTI_DVB_SUPPORT
   endif
else
   STPTI_OPTIONS := -DSTPTI_PTI1_SUPPORT -DSTPTI_PTI3_SUPPORT
endif

$(DVD_TRANSPORT).lib lib($(DVD_TRANSPORT)).a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STPTI_OPTIONS)
        STPTI_CONFIG := $(STPTI_OPTIONS) -DSTPTI_DVB_SUPPORT



stapigat.lib libstapigat.a:\
   DVD_CFLAGS := $(DVD_CFLAGS) -DSTTBX_PRINT 
   STAPIGAT_INCLUDE := stvbi/tests/src $(ST40_IMPORTS)
   STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS)

ifeq "$(DVD_OS)" "LINUX"
    STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS)
else
    STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS) dumpreg
endif

   STAPIGAT_IMPORTS_ST200 := $(ST200_IMPORTS)

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
export STPTI_CONFIG
# Target
ST20_TARGETS = vbi_test.lku

LINUX_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))

ST40_TARGETS := $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))

ST200_TARGETS = $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

#Add Flag for map file
ifeq "$(BUILDING_FOR)" "ST20"
LKFLAGS= -M $(basename $@).map
endif

# Build general application for DRAM
vbi_test_INIT := board_init

ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(IMPORT_LIBS) $(ST20_OBJS)
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
	-$(RM) $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
ifeq "$(BUILDING_FOR)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(BUILDING_FOR)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else
include $(DVD_MAKE)/builddir.mak
endif

# End of makefile






