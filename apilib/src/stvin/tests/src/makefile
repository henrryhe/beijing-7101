########################
# STVIN tests makefile #
########################
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stsys stapigat/src  stapigat/src/linux

# Concat include path
#--------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../src
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../src/hal


COMMON_IMPORTS := stcommon stgxobj stevt testtool stavmem stvout
COMMON_IMPORTS += stvtg stdenc stlayer stvmix stvid stvin stos
COMMON_IMPORTS += stboot stpio sttbx stuart

ST20_IMPORTS := $(COMMON_IMPORTS)
ST40_IMPORTS := $(COMMON_IMPORTS)

LINUX_IMPORTS := stapigat testtool stdenc stgxobj stvtg stvmix
LINUX_IMPORTS += stlayer stvout stevt stavmem

ST20_IMPORTS += stclkrv stpwm sti2c
ST40_IMPORTS += stclkrv stpwm sti2c
LINUX_IMPORTS += stclkrv stpio sti2c

ifneq (,$(findstring $(DVD_BACKEND),7710 ))
endif

ifneq (,$(findstring $(DVD_BACKEND),7100 7109))
endif


ifneq (,$(findstring $(DVD_BACKEND),7100 7109))
    ST40_IMPORTS += stfdma $(DVD_TRANSPORT) stmerge
    LINUX_IMPORTS += stfdma $(DVD_TRANSPORT) stmerge
else
    ifeq "$(DVD_FRONTEND)" "7710"
        ST20_IMPORTS += stfdma stmerge $(DVD_TRANSPORT)lt
        LINUX_IMPORTS += stfdma stmerge $(DVD_TRANSPORT)lt
    else
        ST20_IMPORTS += $(DVD_TRANSPORT)
        LINUX_IMPORTS += $(DVD_TRANSPORT)
    endif
endif

ifeq ($(STAVMEM_NO_FDMA),)
  ifeq "$(DVD_FRONTEND)" "5517"
    STAVMEM_USE_FDMA := 1
  endif
endif
ifneq ($(STAVMEM_USE_FDMA),)
    ST20_IMPORTS += stfdma
endif

ifneq (,$(findstring $(DVD_BACKEND),7015 7020))
    ST20_IMPORTS += stintmr
endif

ifneq (,$(findstring $(DVD_FRONTEND),5517))
    ST20_IMPORTS += stcfg sttsmux
endif

ifneq (,$(findstring $(DVD_FRONTEND),5514))
    ST20_IMPORTS += stgpdma
endif

ifeq "$(DVD_BACKEND)" "GX1"
    ST40_IMPORTS += stintmr
endif

# Last import is stapigat
ST20_IMPORTS += stapigat
ST40_IMPORTS += stapigat
LINUX_IMPORTS += stvin stvid stcommon stos

# Object for ST20
COMMON_OBJS := vin_test.tco

ST20_OBJS := $(COMMON_OBJS)

CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)
# for stavmem kernel mode
ifeq "$(DVD_OS)" "LINUX"
  CFLAGS += -DAVMEM_KERNEL_MODE
endif


STAPIGAT_FLAGS :=
STTBX_FLAGS :=

# Object for ST40
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Define DB573 if 7020 STEM is present
ifeq "$(DVD_BACKEND)" "7020"
    PLATFORM_WITH_7020STEM_SUPPORTED = mb314 mb382
    ifeq "$(findstring $(DVD_PLATFORM), $(PLATFORM_WITH_7020STEM_SUPPORTED))" "$(DVD_PLATFORM)"
        USE_STEM7020 = TRUE
    endif
endif

ifeq ($(DVD_STVIN),MASTER)
    CFLAGS += -D ST_MasterDigInput
    STAPIGAT_FLAGS += -D ST_MasterDigInput
endif

ifneq ($(STVID_DEBUG_GET_STATISTICS),)
    CFLAGS += -D STVID_DEBUG_GET_STATISTICS
    STAPIGAT_FLAGS += -D STVID_DEBUG_GET_STATISTICS
endif

ifeq ($(STVIN_DCACHE_DISABLED),)
    STAPIGAT_FLAGS += -D DCACHE_ENABLE
endif
ifeq ($(STVIN_ICACHE_DISABLED),)
    STAPIGAT_FLAGS += -D ICACHE_ENABLE
endif

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
        # For DVB, use STPTI4_DVB-REL_xxx ; for DirecTV, use STPTI4_DTV-REL_xxx
        ifeq ($(DVD_SERVICE),DIRECTV)
            CFLAGS += -D STPTI_DTV_SUPPORT -D STVID_DIRECTV
            STAPIGAT_FLAGS += -D STPTI_DTV_SUPPORT -D STVID_DIRECTV
            STPTI_OPTIONS += -D STPTI_DTV_SUPPORT
            STCLKRV_OPTIONS += -D STPTI_DTV_SUPPORT
        else
            CFLAGS += -D STPTI_DVB_SUPPORT
            STAPIGAT_FLAGS += -D STPTI_DVB_SUPPORT
            STPTI_OPTIONS += -D STPTI_DVB_SUPPORT
            STCLKRV_OPTIONS += -D STPTI_DVB_SUPPORT
        endif
    else
        CFLAGS += -D STPTI_DVB_SUPPORT -D STPTI_DTV_SUPPORT
        STAPIGAT_FLAGS += -D STPTI_DVB_SUPPORT -D STPTI_DTV_SUPPORT
        STPTI_OPTIONS += -D STPTI_DVB_SUPPORT -D STPTI_DTV_SUPPORT
        STCLKRV_OPTIONS += -D STPTI_DVB_SUPPORT -D STPTI_DTV_SUPPORT
        ifeq ($(DVD_SERVICE),DIRECTV)
            CFLAGS += -D STVID_DIRECTV
            STAPIGAT_FLAGS += -D STVID_DIRECTV
        endif
    endif
endif

# redirect standard input/output to UART
ifneq ($(STVID_USE_UART_AS_STDIO),)
	STTBX_FLAGS += -DSTTBX_FILTER
	CFLAGS += -DSTVID_USE_UART_AS_STDIO
	STAPIGAT_FLAGS += -DSTVID_USE_UART_AS_STDIO -DSTTBX_FILTER
else
	STTBX_FLAGS += -DSTTBX_NO_UART
	STAPIGAT_FLAGS += -DSTTBX_NO_UART
endif

ifneq ($(STVID_TRACE_BUFFER_UART),)
    CFLAGS += -D STVID_TRACE_BUFFER_UART=$(STVID_TRACE_BUFFER_UART)
    STAPIGAT_FLAGS += -D STVID_TRACE_BUFFER_UART=$(STVID_TRACE_BUFFER_UART)
endif


stapigat.lib libstapigat.a :\
   DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
   STAPIGAT_INCLUDE := stvin/tests/src stvid/tests stcommon
   STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS) stextvin
   ifeq "$(DVD_OS)" "LINUX"
       STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS) stextvin
   else
       STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS) dumpreg stextvin
   endif

export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40

 # STAVMEM & STFDMA are special in linux - no libraries are created so we must not add
 # the libraries to the link command. The following link option suppresses this.
 # This only works currently on LINUX.

LINUX_libstavmem.a_NOLINK := 1
LINUX_libstfdma.a_NOLINK := 1

# Specific mb290 options
ifeq "$(DVD_PLATFORM)" "mb290"
    STCOMMON_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
    STPTI_OPTIONS += -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
    STVIN_OPTIONS := -DSTVIN_WA_SAA7114
endif

ifneq ($(USE_STEM7020),)
    ifeq "$(DVD_FRONTEND)" "5514"
        STCOMMON_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
        STPTI_OPTIONS += -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
    endif
    ifeq "$(DVD_FRONTEND)" "5517"
        STCOMMON_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_5517_GENERIC_ADDRESSES
        STPTI_OPTIONS += -DREMOVE_GENERIC_ADDRESSES -DUSE_5517_GENERIC_ADDRESSES
    endif
endif

stvin.lib libstvin.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STVIN_OPTIONS)

stuart.lib libstuart.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTUART_TIMER_TASK_PRIORITY=3

stvid.lib libstvid.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTVID_DEBUG_GET_STATISTICS -DSTVID_TASK_PRIORITY_DECODE=10 -DSTVID_TASK_PRIORITY_DISPLAY=12 -DSTVID_TASK_PRIORITY_ERROR_RECOVERY=9 -DSTVID_TASK_PRIORITY_TRICKMODE=11 -DSTVID_TASK_PRIORITY_INJECTERS=9

sttbx.lib libsttbx.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STTBX_FLAGS)

stcommon.lib libstcommon.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STCOMMON_OPTIONS)

stclkrv.lib libstclkrv.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STCLKRV_OPTIONS)

stlayer.lib libstlayer.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_GAMMAGFX_TASK_PRIORITY=7 $(STLAYER_OPTIONS)

$(DVD_TRANSPORT).lib lib$(DVD_TRANSPORT).a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STPTI_OPTIONS)
        STPTI_CONFIG := $(STPTI_OPTIONS)

stvtg.lib libstvtg.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTVTG_TASK_PRIORITY_IT_PROCESSING=15 -DSTVTG_NOTIFY_VSYNC_UNDER_IT

ifeq ($(DVD_PLATFORM), mb317b)
VINTEST_MODULES = saa7114
endif
SUBDIRS += $(VINTEST_MODULES)

ifeq "$(DVD_OS)" "LINUX"
DVD_CFLAGS := $(DVD_CFLAGS) -D$(LINUX_ARCHITECTURE)
endif

export DVD_CFLAGS
export STPTI_CONFIG

# For output of toolbox
CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT

INCLUDE_PATH += $(addprefix -I ../../,$(VINTEST_MODULES))

# Target
ST20_TARGETS = vin_test.lku
ST20_TARGETS_OTHER = vin_test.dbg
ifeq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))
else
ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
endif
ST40_TARGETS_OTHER =

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

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
ifdef NATIVE_COMPIL
	FULLDIR_EMBX := $(RPC_ROOT)/lib/os21/native
endif


EMBX_IMPORTS = embxmailbox embxshm embxshell mme_host
IMPORT_EMBX_LIBS = $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(EMBX_IMPORTS)))
$(IMPORT_EMBX_LIBS):
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH), $(FULLDIR_EMBX)/$(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(LIBNAME_FROM_AT_F)))) \
		$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_$(BUILDING_FOR)_EXPORTS)) > $(NULL)
endif
endif

# Build general application for DRAM
vin_test_INIT := board_init

ifneq (,$(findstring $(DVD_BACKEND),7100 7109))
$($(BUILDING_FOR)_TARGETS): $(IMPORT_LIBS) $($(BUILDING_FOR)_OBJS) $(SUBDIR_LIBS) $(IMPORT_EMBX_LIBS)
else
$($(BUILDING_FOR)_TARGETS): $(IMPORT_LIBS) $($(BUILDING_FOR)_OBJS) $(SUBDIR_LIBS)
endif
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)
ifeq "$(DVD_OS)" "LINUX"
	$(CP) $(ST40_TARGETS) $(DVD_EXPORTS)/bin/$(ST40_TARGETS).exe
endif


clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $(TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS) $($(ARCHITECTURE)_TARGETS_OTHER)
ifeq "$(ARCHITECTURE)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(ARCHITECTURE)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(ARCHITECTURE)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample ST20 makefile

