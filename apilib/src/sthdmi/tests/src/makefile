#########################
# STHDMI tests makefile #
#########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon

HEADER_IMPORTS := stsys stapigat/src staudlx stcommon stos stapigat/src/patch
COMMON_IMPORTS := stapigat stcommon stos stevt stgxobj testtool stdenc stvtg stvout stclkrv stvmix stvid stavmem stlayer sthdmi

ifeq "$(STHDMI_CEC)" "TRUE"
        DVD_CFLAGS :=$(DVD_CFLAGS)  -DSTHDMI_CEC
endif

ifeq "$(HDMI_HDCP_ON)" "TRUE"
        DVD_CFLAGS :=$(DVD_CFLAGS)  -DHDMI_HDCP_ON
endif

ifneq "$(DVD_OS)" "LINUX"
ifeq "$(STHDMI_UART_IO)" "TRUE"
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


ST20_IMPORTS := $(COMMON_IMPORTS) stboot stpio sttbx
ST40_IMPORTS := $(COMMON_IMPORTS) stboot stpio sttbx

ifneq (,$(findstring $(DVD_BACKEND),7710 7100 7109))
        ST20_IMPORTS += stpwm
        ST40_IMPORTS += stpwm
endif

LINUX_IMPORTS := $(COMMON_IMPORTS) stfdma stpio


TARGET_WITH_STI2C = mb391 mb411 mb519
ifeq "$(findstring $(DVD_PLATFORM), $(TARGET_WITH_STI2C))" "$(DVD_PLATFORM)"
  ST20_IMPORTS += sti2c
  ST40_IMPORTS += sti2c
  LINUX_IMPORTS += sti2c
endif



ifneq (,$(findstring $(DVD_BACKEND),7710 ))
        ST20_IMPORTS += stfdma stmerge staud
endif

ifeq "$(DVD_OS)" "LINUX"
ifneq ($(STVID_ENABLE_AUDIO),)
          ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
              LINUX_IMPORTS += staudlx
          endif
endif
else
ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
            ST40_IMPORTS += staudlx
endif
endif


ifneq (,$(findstring $(DVD_BACKEND),7100 7109 7200))
ifneq (,$(findstring $(DVD_BACKEND),7100 7109))

   ifeq ($(STVID_NO_PTI),)
                ST40_IMPORTS += stmerge
                LINUX_IMPORTS += stmerge
    endif
endif
        # Under Linux, FDMA is a kernel module and must not be compiled in User mode.
ifneq "$(DVD_OS)" "LINUX"
        ST40_IMPORTS += stfdma
endif
endif

ifeq ($(STVID_NO_PTI),)
ifneq (,$(findstring $(DVD_FRONTEND),5105 5188 7710))
ST20_IMPORTS += $(DVD_TRANSPORT)lt
else
ST20_IMPORTS += $(DVD_TRANSPORT)
ST40_IMPORTS += $(DVD_TRANSPORT)
LINUX_IMPORTS += $(DVD_TRANSPORT)
endif
endif


# disable pti use in stapigat
ifneq ($(STVID_NO_PTI),)
        STAPIGAT_FLAGS += -DSTPTI_UNAVAILABLE
	ifeq ($(STVID_NO_CLKRV),)
                ifneq ($(STCLKRV_NO_PTI),)
                        STAPIGAT_FLAGS += -DSTCLKRV_NO_PTI
                endif
        endif

endif

BACKEND_USE_CLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(BACKEND_USE_CLKRV))" "$(DVD_BACKEND)"
  STAPIGAT_FLAGS += -DSTVTG_USE_CLKRV
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
    ifneq (,$(findstring $(DVD_FRONTEND),7100 7109 7200))
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
    	ifeq ($(DVD_SERVICE),DIRECTV)
        	CFLAGS += -D STVID_DIRECTV
        	STAPIGAT_FLAGS += -D STVID_DIRECTV
    	endif
	endif
endif

ifneq ($(STVID_DEBUG_GET_STATISTICS),)
    CFLAGS += -D STVID_DEBUG_GET_STATISTICS
    STAPIGAT_FLAGS += -D STVID_DEBUG_GET_STATISTICS
endif

# Object for ST20
ST20_OBJS := hdmi_test.tco

# Object for ST40
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))


ifeq "$(DVD_BACKEND)" "7710"
  ifeq "$(STI7710_CUT2x)" "TRUE"
    CFLAGS := $(CFLAGS) -DSTI7710_CUT2x
  endif
  DVD_CFLAGS := $(DVD_CFLAGS) -DHDMI_I2C
endif


CFLAGS := $(CFLAGS) $(OPTIONAL_CFLAGS)


ifeq "$(DVD_OS)" "LINUX"
DVD_CFLAGS := $(DVD_CFLAGS) -DAVMEM_KERNEL_MODE
CFLAGS := $(CFLAGS) -DAVMEM_KERNEL_MODE
DVD_CFLAGS := $(DVD_CFLAGS)  -DSTTBX_PRINT
endif

# For output of toolbox
ifeq "$(STHDMI_UART_IO)" "TRUE"
  CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT
else
  CFLAGS := $(CFLAGS) -D STTBX_PRINT -D STTBX_INPUT -D STTBX_NO_UART
  DVD_CFLAGS := $(DVD_CFLAGS) -D STTBX_NO_UART
endif




ifneq ($(DEBUG),)
  STVOUT_ICACHE_DISABLED := 1
  STHDMI_DCACHE_DISABLED := 1
endif
ifeq ($(STHDMI_DCACHE_DISABLED),)
  STAPIGAT_FLAGS +=  -DDCACHE_ENABLE
endif
ifeq ($(STHDMI_ICACHE_DISABLED),)
  STAPIGAT_FLAGS += -DICACHE_ENABLE
endif

ifeq "$(DVD_OS)" "LINUX"
STAPIGAT_INCLUDE_PATH += stvid/src/linux stcommon
#STAPIGAT_INCLUDE_PATH += staudlt $(STAPIGAT_INCLUDE_PATH)
endif

ifneq ($(USE_AUDIO),)
    CFLAGS += -D USE_AUDIO
    STAPIGAT_FLAGS += -D USE_AUDIO
endif

stapigat.lib libstapigat.a: \
    DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
    STAPIGAT_INCLUDE := sthdmi/tests/src stvid/tests staudlx $(STAPIGAT_INCLUDE_PATH)
    STAPIGAT_IMPORTS_ST20 := $(ST20_IMPORTS)
ifeq "$(DVD_OS)" "LINUX"
    STAPIGAT_IMPORTS_ST40 := $(LINUX_IMPORTS)
else
    STAPIGAT_IMPORTS_ST40 := $(ST40_IMPORTS)
endif


sthdmi.lib libsthdmi.a :\
        DVD_CFLAGS := $(DVD_CFLAGS)

stclkrv.lib libstclkrv.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STCLKRV_OPTIONS)

$(DVD_TRANSPORT).lib lib$(DVD_TRANSPORT).a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STPTI_OPTIONS)
        STPTI_CONFIG := $(STPTI_OPTIONS)

staud.lib libstaud.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP

ifeq "$(DVD_BACKEND)" "7200"
    ifndef MULTICOM_COMPANION_CONFIG
        export MULTICOM_COMPANION_CONFIG=0x07
    endif
endif

export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40


# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS
export STPTI_CONFIG


ifneq ($(DEBUG),)
  STHDMI_ICACHE_DISABLED := 1
  STHDMI_DCACHE_DISABLED := 1
endif
ifeq ($(STVID_DCACHE_DISABLED),)
  CFLAGS := $(CFLAGS) -D DCACHE_ENABLE
endif
ifeq ($(STVID_ICACHE_DISABLED),)
  CFLAGS := $(CFLAGS) -D ICACHE_ENABLE
endif


# Target
ST20_TARGETS = hdmi_test.lku
ST20_TARGETS_OTHER = hdmi_test.dbg

ST40_TARGETS_OTHER =
ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
LINUX_TARGETS = $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))


#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build general application for DRAM
hdmi_test_INIT := board_init

ifeq "$(BUILDING_FOR)" "ST20"
	LKFLAGS = -M $(basename $@).map
endif


#------------------------------ Added from stvid tests src
BOARDSPECS=
ifneq (,$(findstring $(DVD_BACKEND),7100 7109 7200))
ifeq "$(DVD_OS)" "LINUX"
IMPORT_EMBX_LIBS =
else
# FULLDIR_EMBX = $(RPC_ROOT)/lib/os21/$(ARCHITECTURE)
# !! WARNING !! Can't use $(ARCHITECTURE) in FULLDIR_EMBX because architecture is
# upper case and MULTICOM directories are lower case -> issue for use under Unix
ifeq "$(BUILDING_FOR)" "ST40"
	FULLDIR_EMBX := $(RPC_ROOT)/lib/os21/st40
        ifeq "$(DVD_BACKEND)" "7109"
    		BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7109
        else
            ifeq "$(DVD_BACKEND)" "7200"
	       	 export USE_MBOARD=board_$(DVD_PLATFORM)_lmi0_$(MULTICOM_COMPANION_CONFIG)
     		 BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7200
             else
    		 BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs
             endif
        endif

endif
ifeq "$(BUILDING_FOR)" "ST200"
	FULLDIR_EMBX := $(RPC_ROOT)/lib/os21/st200
endif
ifeq "$(BUILDING_FOR)" "ST20"
	FULLDIR_EMBX := $(RPC_ROOT)/lib/os20/st20
endif


EMBX_IMPORTS = embxmailbox embxshm embxshell mme_host
IMPORT_EMBX_LIBS = $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(EMBX_IMPORTS)))
$(IMPORT_EMBX_LIBS):
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH), $(FULLDIR_EMBX)/$(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(LIBNAME_FROM_AT_F)))) \
                $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_$(BUILDING_FOR)_EXPORTS)) > $(NULL)
endif
endif
#------------------------------ Added from stvid tests src

ifeq "$(DVD_OS)" "LINUX"
  # STAVMEM is special in linux - no library is created so we must not add
  # STFDMA is special in linux - no library is created so we must not add

  # the library to the link command. The following link option suppresses this.
  # This only works currently on LINUX.
  LINUX_libstfdma.a_NOLINK := 1
  LINUX_libstavmem.a_NOLINK := 1
endif

$(ST20_TARGETS): $(ST20_OBJS) $(IMPORT_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE)

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(IMPORT_LIBS) $(IMPORT_EMBX_LIBS)
	@$(ECHO) Linking $@
	$(LINK_EXECUTABLE) -Xlinker -Map=../../hdmi_test.map $(BOARDSPECS)
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
	-$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample makefile
