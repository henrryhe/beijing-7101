##########################
# STCC Test Application #
##########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
# -------------------------------------------------

HEADER_IMPORTS := stos stsys  stapigat/src

COMMON_IMPORTS := stapigat stos stcommon testtool stavmem stgxobj stevt stdenc stclkrv stvtg stvout stlayer stvmix
COMMON_IMPORTS += stcc stvbi

LINUX_IMPORTS := $(COMMON_IMPORTS) stpio

ifeq ($(STCC_NO_PTI),)
	ifneq (,$(findstring $(DVD_FRONTEND),5105 5107 7710))
		COMMON_IMPORTS += $(DVD_TRANSPORT)lt
		LINUX_IMPORTS += $(DVD_TRANSPORT)lt
    	else
        ifneq (,$(findstring $(DVD_FRONTEND),5188))
            COMMON_IMPORTS += st$(DVD_TRANSPORT)
		LINUX_IMPORTS += st$(DVD_TRANSPORT)
        else
            COMMON_IMPORTS += $(DVD_TRANSPORT)
            LINUX_IMPORTS += $(DVD_TRANSPORT)
        endif
    	endif
endif

TARGET_WITH_NO_STPWM = 5105 5301 5107
ifneq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_NO_STPWM))" "$(DVD_FRONTEND)"
	COMMON_IMPORTS += stpwm
endif

TARGET_WITH_STCFG = 5514 5516 5517 5528 5100 5105 5107
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STCFG))" "$(DVD_FRONTEND)"
	COMMON_IMPORTS += stcfg
endif

#TARGET_WITH_VALITLS = 5514 5516 5517 5528 5100 5301
#ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_VALITLS))" "$(DVD_FRONTEND)"
#  COMMON_IMPORTS += valitls
#endif

TARGET_WITH_STDISP = 5100 5301 5105 5107
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STDISP))" "$(DVD_FRONTEND)"
	COMMON_IMPORTS += stdisp
endif

TARGET_WITH_COMPO = 5100 5301 5105 5107
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_COMPO))" "$(DVD_FRONTEND)"
	COMMON_IMPORTS +=compo
endif

TARGET_WITH_STMERGE = 5100 5528 7710 7100 7109
ifeq ($(STCC_NO_PTI),)
	ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STMERGE))" "$(DVD_FRONTEND)"
      	COMMON_IMPORTS += stmerge
		LINUX_IMPORTS += stmerge
	endif
endif

TARGET_WITH_STI2C = mb282b mb314 mb361 mb382 mb295 mb376 mb390 espresso mb391 mb411 mb400 mb436
ifeq "$(findstring $(DVD_PLATFORM), $(TARGET_WITH_STI2C))" "$(DVD_PLATFORM)"
  COMMON_IMPORTS += sti2c
endif

TARGET_WITH_STINTMR = 7015 7020 GX1
ifeq "$(findstring $(DVD_BACKEND), $(TARGET_WITH_STINTMR))" "$(DVD_BACKEND)"
  COMMON_IMPORTS += stintmr
endif

FRONTEND_WITH_LLI = 5510 5512 5514 5516 5517 TP3
ifeq "$(findstring $(DVD_FRONTEND), $(FRONTEND_WITH_LLI))" "$(DVD_FRONTEND)"
	COMMON_IMPORTS += stlli
endif

TARGET_WITH_STTSMUX = 5514 5516 5517
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STTSMUX))" "$(DVD_FRONTEND)"
  COMMON_IMPORTS += sttsmux
endif

TARGET_WITH_STGPDMA = 5514 5528
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STGPDMA))" "$(DVD_FRONTEND)"
  COMMON_IMPORTS += stgpdma
endif

TARGET_WITH_STFDMA = 5517 5100 7710 7100 5301 5105 5107 7109
ifeq "$(findstring $(DVD_FRONTEND), $(TARGET_WITH_STFDMA))" "$(DVD_FRONTEND)"
  COMMON_IMPORTS += stfdma
endif

# Add STVID as last IMPORTS
COMMON_IMPORTS += stvid
LINUX_IMPORTS += stvid

ST40_IMPORTS := $(COMMON_IMPORTS) stboot stpio stuart sttbx
ST20_IMPORTS := $(COMMON_IMPORTS) stboot stpio stuart sttbx
ST200_IMPORTS := $(COMMON_IMPORTS) stboot stpio stuart sttbx

# Sources of the components
# -------------------------

# Object for ST20
ST20_OBJS := cc_cmd.tco decoder.tco

# Object for ST200
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Flags and options
# -----------------


# For compilation of pti+link or stpti
CFLAGS += $(OPTIONAL_CFLAGS)

# DVD_SERVICE specific
ifeq "$(DVD_TRANSPORT)" "stpti"
   CFLAGS += -D STPTI_PTI1_SUPPORT -D STPTI_PTI3_SUPPORT -D STPTI_DVB_SUPPORT
   STAPIGAT_FLAGS += -D STPTI_PTI1_SUPPORT -D STPTI_PTI3_SUPPORT -D STPTI_DVB_SUPPORT
   STCLKRV_OPTIONS += -D STPTI_PTI1_SUPPORT -D STPTI_PTI3_SUPPORT -D STPTI_DVB_SUPPORT
   STPTI_OPTIONS := -D STPTI_PTI1_SUPPORT -D STPTI_PTI3_SUPPORT -D STPTI_DVB_SUPPORT
    ifeq ($(DVD_SERVICE),DIRECTV)
        CFLAGS += -D STVID_DIRECTV
        STAPIGAT_FLAGS += -D STVID_DIRECTV -D STPTI_DTV_SUPPORT
        STPTI_OPTIONS += -D STPTI_DTV_SUPPORT
        STCLKRV_OPTIONS += -D STPTI_DTV_SUPPORT
    endif
endif
ifeq "$(DVD_TRANSPORT)" "stpti4"
    ifneq (,$(findstring $(DVD_FRONTEND),7100 7109))
 # no DTV for now on PTI4 for 7100, waiting for a new release...
    	CFLAGS += -D STPTI_DVB_SUPPORT
    	STAPIGAT_FLAGS += -D STPTI_DVB_SUPPORT
    	STPTI_OPTIONS += -D STPTI_DVB_SUPPORT
    	STCLKRV_OPTIONS += -D STPTI_DVB_SUPPORT
    else
    	CFLAGS += -D STPTI_DVB_SUPPORT -D STPTI_DTV_SUPPORT
    	STAPIGAT_FLAGS += -D STPTI_DVB_SUPPORT -D STPTI_DTV_SUPPORT
    	STPTI_OPTIONS += -D STPTI_DVB_SUPPORT -D STPTI_DTV_SUPPORT
    	STCLKRV_OPTIONS += -D STPTI_DVB_SUPPORT -D STPTI_DTV_SUPPORT
    endif
endif

# For STAPIGAT, caches, injection workaround
ifneq ($(DEBUG),)
    STVID_ICACHE_DISABLED :=
    STVID_DCACHE_DISABLED :=
endif

ifeq ($(STVID_DCACHE_DISABLED),)
    STAPIGAT_FLAGS += -D DCACHE_ENABLE
endif
ifeq ($(STVID_ICACHE_DISABLED),)
    STAPIGAT_FLAGS += -D ICACHE_ENABLE
endif
# ICache must be enabled for st200
ifeq "$(BUILDING_FOR)" "ST200"
    STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

ifneq ($(STVID_DEBUG_GET_STATISTICS),)
    CFLAGS += -D STVID_DEBUG_GET_STATISTICS
    STAPIGAT_FLAGS += -D STVID_DEBUG_GET_STATISTICS
endif

ifneq ($(STVID_WRAPPER_OLDARCH),)
    CFLAGS += -D STVID_WRAPPER_OLDARCH
    STAPIGAT_FLAGS += -D STVID_WRAPPER_OLDARCH
endif

ifneq ($(USE_HDD),)
    CFLAGS += -D USE_HDD
    STAPIGAT_FLAGS += -D USE_HDD
endif
ifneq ($(USE_OLD_HDD),)
    CFLAGS += -D USE_HDD
    STAPIGAT_FLAGS += -D USE_HDD
endif

# Add define for test application to be informed of hardware error
ifneq ($(STVID_HARDWARE_ERROR_EVENT),)
    CFLAGS += -D STVID_HARDWARE_ERROR_EVENT
    STAPIGAT_FLAGS += -D STVID_HARDWARE_ERROR_EVENT
endif

# DVD_SERVICE  specific
ifeq ($(DVD_SERVICE),DIRECTV)
    CFLAGS += -D STVID_DIRECTV -D STPTI_DTV_SUPPORT
    STAPIGAT_FLAGS += -D STVID_DIRECTV -D STPTI_DTV_SUPPORT
    STPTI_CONFIG += -DSTPTI_DTV_SUPPORT
    STCLKRV_OPTIONS += -D SERVICE_DIRECTV -D STPTI_DTV_SUPPORT
endif
ifeq ($(DVD_SERVICE),DVB)
     STPTI_CONFIG += -DSTPTI_DVB_SUPPORT
endif

# for Old STVIN
ifeq ($(DVD_STVIN),MASTER)
    CFLAGS += -D ST_MasterDigInput
    STAPIGAT_FLAGS += -D ST_MasterDigInput
endif

# disable pti use in stapigat
ifneq ($(STCC_NO_PTI),)
    CFLAGS += -DSTPTI_UNAVAILABLE
    STAPIGAT_FLAGS += -D STPTI_UNAVAILABLE
endif


# Specific mb290 option
ifeq "$(DVD_PLATFORM)" "mb290"
    STCOMMON_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
    STPTI_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
endif

# Specific compilation flags

ifeq "$(DVD_OS)" "LINUX"
STAPIGAT_INCLUDE_PATH += stvid/src/linux
STAPIGAT_INCLUDE_PATH += stvid/tests
STAPIGAT_INCLUDE_PATH += stfdma
STAPIGAT_INCLUDE_PATH += stmerge
STAPIGAT_INCLUDE_PATH += stpti4
endif

staud.lib libstaud.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP

stblit.lib libstblit.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTBLIT_MASTER_TASK_STACK_PRIORITY=4 -DSTBLIT_SLAVE_TASK_STACK_PRIORITY=4 -DSTBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY=5

stboot.lib libstboot.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STBOOT_OPTIONS)

ifeq "$(DVD_OS)" "LINUX"
stcc.lib libstcc.a:\
	DVD_CFLAGS := $(DVD_CFLAGS) -DSTTBX_REPORT -DSTTBX_PRINT
else
stcc.lib libstcc.a:\
	DVD_CFLAGS :=$(DVD_CFLAGS) -DSTTBX_REPORT -DSTTBX_PRINT -DCLOSED_CAPTION_TEST_MODE
endif

stclkrv.lib libstclkrv.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STCLKRV_OPTIONS)

stcommon.lib libstcommon.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STCOMMON_OPTIONS)

stlayer.lib libstlayer.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_GAMMAGFX_TASK_PRIORITY=7 $(STLAYER_OPTIONS)

ifeq ($(STCC_NO_PTI),)
$(DVD_TRANSPORT).lib $(DVD_TRANSPORT)lib.a:\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STPTI_OPTIONS)
      STPTI_CONFIG := $(STPTI_OPTIONS)
endif

sttbx.lib libsttbx.a : \
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTTBX_NO_UART

stuart.lib libstuart.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTUART_TIMER_TASK_PRIORITY=3

sttuner.lib libsttuner.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTTBX_REPORT -DSTTBX_PRINT -DSTTBX_INPUT
# Priority for video is Display > Trickmode > Decode > Error Recovery
stvid.lib libstvid.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTVID_TASK_PRIORITY_DECODE=10 -DSTVID_TASK_PRIORITY_DISPLAY=12 -DSTVID_TASK_PRIORITY_ERROR_RECOVERY=9 -DSTVID_TASK_PRIORITY_TRICKMODE=11 -DMULTICOM

stvtg.lib libstvtg.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTVTG_TASK_PRIORITY_IT_PROCESSING=15

# STAVMEM is special in linux - no library is created so we must not add
# the library to the link command. The following link option suppresses this.
# This only works currently on LINUX.
LINUX_libstavmem.a_NOLINK := 1


stavmem.lib libstavmem.a:\
        STAVMEM_NO_GPDMA=1
        STAVMEM_NO_FDMA=1
export STAVMEM_NO_GPDMA
export STAVMEM_NO_FDMA

stapigat.lib libstapigat.a: \
    DVD_CFLAGS :=$(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := stcc/tests/src $(ST40_IMPORTS) $(STAPIGAT_INCLUDE_PATH)
    STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS)
#dumpreg
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


# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS
export STPTI_CONFIG

# For output of toolbox
CFLAGS += -DSTTBX_REPORT -DSTTBX_PRINT -DSTTBX_INPUT

ifeq ($(FLOATING_POINT), fp)
        LKFLAGS += -fp
endif


# Target
# ------

ST20_TARGETS = cc_test.lku
ST40_TARGETS= $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
ST200_TARGETS= $(call ST20EXE_TO_ST200EXE,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build general application for DRAM
cc_test_INIT := board_init
#
BOARDSPECS=

ifneq (,$(findstring $(DVD_BACKEND),7100 7109))
ifeq "$(DVD_OS)" "LINUX"
IMPORT_EMBX_LIBS =
else
# FULLDIR_EMBX = $(RPC_ROOT)/lib/os21/$(BUILDING_FOR)
# !! WARNING !! Can't use $(BUILDING_FOR) in FULLDIR_EMBX because architecture is
# upper case and MULTICOM directories are lower case -> issue for use under Unix
ifeq "$(BUILDING_FOR)" "ST40"
        FULLDIR_EMBX := $(RPC_ROOT)/lib/os21/st40
        ifeq "$(DVD_BACKEND)" "7109"
            BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7109
        else
            BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs
        endif
endif
ifeq "$(BUILDING_FOR)" "ST200"
        FULLDIR_EMBX := $(RPC_ROOT)/lib/os21/st200
endif
ifeq "$(BUILDING_FOR)" "ST20"
        FULLDIR_EMBX := $(RPC_ROOT)/lib/os20/st20
endif



EMBX_IMPORTS = embxmailbox embxshm embxshell mme_host

#EMBX_IMPORTS = embxshell mme_host
IMPORT_EMBX_LIBS = $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(EMBX_IMPORTS)))
$(IMPORT_EMBX_LIBS):
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH), $(FULLDIR_EMBX)/$(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(LIBNAME_FROM_AT_F))))
                $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_$(BUILDING_FOR)_EXPORTS)) > $(NULL)
endif
endif
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(IMPORT_LIBS) $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(IMPORT_LIBS) $(ST40_OBJS) $(SUBDIR_LIBS) $(IMPORT_EMBX_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) $(BOARDSPECS)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(IMPORT_LIBS) $(LINUX_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(IMPORT_LIBS) $(ST200_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
endif
# Build test application but ignore dependencies
test: $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	$(CC) $(LIBRARIES) -T $(DVD_PLATFORM).cfg \
	$(ST20_LKFLAGS) $(cc_test_LKFLAGS) $(cc_test_ST20_LKFLAGS) \
        $^ $(IMPORT_LIBS) $(OSLIBS) -p $(vid_test_INIT) -o $(ST20_TARGETS)

# Build stcc driver and test application but ignore dependencies
no_imports: stcc.lib $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	$(CC) $(LIBRARIES) -T $(DVD_PLATFORM).cfg \
	$(ST20_LKFLAGS) $(cc_test_LKFLAGS) $(cc_test_ST20_LKFLAGS) \
        $^ $(IMPORT_LIBS) $(OSLIBS) -p $(cc_test_INIT) -o $(ST20_TARGETS)

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








