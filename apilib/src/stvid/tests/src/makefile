##########################
# STVID Test Application #
##########################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
# -------------------------------------------------

HEADER_IMPORTS := stsys stapigat/src

IMPORTS := stapigat stevt testtool stdenc
IMPORTS += stgxobj stvtg stvmix stlayer stvout stavmem stpio sttbx

ifneq "$(DVD_OS)" "LINUX"
IMPORTS += stboot stuart
endif

# Add clock recovery only if needed
ifeq ($(STVID_NO_CLKRV),)
IMPORTS += stclkrv
endif

# Don't include sti2c for first snapshot on STi5301 and STi5525
ifeq (,$(findstring $(DVD_BACKEND), 5301 5525))
    IMPORTS += sti2c
endif

ifneq "$(DVD_OS)" "LINUX"
# Removed only for first snapshot on STi5301 and also because there is no pwm on STi5105
ifeq (,$(findstring $(DVD_BACKEND), 5301 5105 5107 5188 5525 7200 5162))
    IMPORTS += stpwm
endif
endif

# Define DB573 if 7020 STEM is present
ifeq "$(DVD_BACKEND)" "7020"
    PLATFORM_WITH_7020STEM_SUPPORTED = mb314 mb382 mb376
    ifeq "$(findstring $(DVD_PLATFORM), $(PLATFORM_WITH_7020STEM_SUPPORTED))" "$(DVD_PLATFORM)"
        USE_STEM7020 = TRUE
    endif
endif

# define USE_DELTAPHI_HE if backend, frontend en platform are OK
ifeq "$(DVD_BACKEND)" "DELTAPHI_HE"
    ifeq "$(DVD_FRONTEND)" "5528"
        ifeq "$(DVD_PLATFORM)" "mb376"
            USE_DELTAPHI_HE = TRUE
        endif
    endif
endif

# define USE_DELTAMU_HE if backend, frontend en platform are OK
ifeq "$(DVD_BACKEND)" "DELTAMU_HE"
    ifeq "$(DVD_FRONTEND)" "5528"
        ifeq "$(DVD_PLATFORM)" "mb376"
            USE_DELTAMU_HE = TRUE
        endif
    endif
endif

# add stfdma and mme for 7100 software debug or for DELTAPHI_HE/DELTAMU_HE
ifdef STVID_7100_SOFTWARE_PLATFORM
    IMPORTS += stfdma
else
    ifdef USE_DELTAPHI_HE
        IMPORTS += stfdma
    else
        ifdef USE_DELTAMU_HE
            IMPORTS += stfdma
        endif
    endif
endif


# add stfdma for 7109 software
ifdef STVID_7109_SOFTWARE_PLATFORM
    IMPORTS += stfdma
endif

# add stvin commands for mb295, mb290 and STEM 7020 only
ifneq (,$(findstring $(DVD_PLATFORM),mb391))
        IMPORTS += stvin
endif
ifneq (,$(findstring $(DVD_PLATFORM),mb411))
        IMPORTS += stvin
endif


ifneq (,$(findstring $(DVD_PLATFORM),mb295))
        IMPORTS += stvin
endif
ifneq (,$(findstring $(DVD_PLATFORM),mb290))
        IMPORTS += stvin
endif
ifneq ($(USE_STEM7020),)
    IMPORTS += stvin
endif

# add dvdgr-prj-valitls tools for validation application
ifdef VALID_TOOLS
    IMPORTS += valitls
endif

ifeq ($(STVID_NO_PTI),)
    ifneq (,$(findstring $(DVD_FRONTEND),5105 5107 7710 5162))
        IMPORTS += $(DVD_TRANSPORT)lt
    else
        ifneq (,$(findstring $(DVD_FRONTEND),5188))
            IMPORTS += st$(DVD_TRANSPORT)
        else
            IMPORTS += $(DVD_TRANSPORT)
        endif
    endif
endif

ifneq (,$(findstring $(DVD_FRONTEND),5514 5516 5517))
        IMPORTS += stcfg sttsmux
endif

ifneq (,$(findstring $(DVD_FRONTEND),5528 5100 5105 5107 5188 5162))
        IMPORTS += stcfg
endif

ifneq (,$(findstring $(DVD_FRONTEND),5528 5100 5301 5525 5162))
	ifeq ($(STVID_NO_PTI),)
        IMPORTS += stmerge
	endif
endif

ifneq (,$(findstring $(DVD_FRONTEND),5514 5528))
        IMPORTS += stgpdma
endif

ifneq (,$(findstring $(DVD_BACKEND),7015 7020))
        IMPORTS += stintmr
endif
ifneq (,$(findstring $(DVD_BACKEND),5100 5105 5107 5301 5525 5188 5162))
        IMPORTS += stfdma compo stdisp
endif
ifneq (,$(findstring $(DVD_BACKEND),7710 ))
        IMPORTS += stfdma stmerge
endif
ifneq (,$(findstring $(DVD_BACKEND),7100 7109))
    ifeq ($(STVID_NO_PTI),)
	IMPORTS += stmerge
    endif
endif
ifneq (,$(findstring $(DVD_BACKEND),7100 7109 7200))
    IMPORTS += stfdma
endif

ifeq ($(STAVMEM_NO_FDMA),)
  ifeq "$(DVD_FRONTEND)" "5517"
    STAVMEM_USE_FDMA := 1
  endif
endif
ifneq ($(STAVMEM_USE_FDMA),)
    IMPORTS += stfdma
endif

# Define parameters according the DVD_FRONTEND
FRONTEND_WITH_LLI = 5510 5512 5514 5516 5517 TP3
ifeq "$(findstring $(DVD_FRONTEND), $(FRONTEND_WITH_LLI))" "$(DVD_FRONTEND)"
	IMPORTS += stlli
endif

ifneq ($(STVID_USE_FILESYSTEM),)
    # Define compile flag
    USE_FILESYSTEM = TRUE

    # Define compile flag to use OSPLUS when compiling for OS20 and OS21
    ifneq "$(DVD_OS)" "LINUX"
        USE_OSPLUS = TRUE

        export USE_OSPLUS
    endif

    # If STVID_USE_FILESYSTEM is defined, disable the use of ATAPI devices
    ifeq ($(STVID_NO_HDD),)
	STVID_NO_HDD = 1
        USE_HDD =
        USE_OLD_HDD =
    endif

    export USE_FILESYSTEM
endif

# Define HDD and STATAPI parameters according the DVD_PLATFORM
ifeq ($(STVID_NO_HDD),)
        USE_HDD =
        USE_OLD_HDD =
        ifeq ($(STVID_TEST_OLD_ATAPI),)
                PLATFORM_WITH_HDD_SUPPORTED = mb282b mb275 mb275_64 mb314 mb290 mb361 mb382 mb376 espresso mb390 mb391 mb428 mb411 mb634
                ifeq "$(findstring $(DVD_PLATFORM), $(PLATFORM_WITH_HDD_SUPPORTED))" "$(DVD_PLATFORM)"
                        IMPORTS += statapi
                        USE_HDD = TRUE
                endif
        else
                USE_OLD_HDD = TRUE
        endif
endif

# Define TUNER and STATAPI parameters
ifneq ($(STVID_TUNER),)
        IMPORTS += sttuner
        USE_TUNER = TRUE
endif

ifneq ($(STVID_ENABLE_AUDIO),)
        ifeq "$(DVD_OS)" "LINUX"
        ifneq (,$(findstring $(DVD_BACKEND),7100 7109 7200))
                    IMPORTS += staudlx
	            USE_AUDIO = TRUE
                endif
        else
            IMPORTS += staud
            USE_AUDIO = TRUE
        endif
  endif

# Add stmes library if compiling for secure 7109
ifeq "$(DVD_BACKEND)" "7109"
    ifneq ($(DVD_SECURED_CHIP),)
        ifeq "$(DVD_TOOLSET)" "ST40WINCE"
            LDOPTS += $(DVD_EXPORTS)/stmes.lib
        else
            LDOPTS += $(DVD_EXPORTS)/libstmes.a
        endif
    endif
endif

# Add STVID as last IMPORTS
IMPORTS += stvid

# Very last IMPORTS
IMPORTS += stcommon stos

ST40_IMPORTS = $(IMPORTS)
ST200_IMPORTS = $(ST40_IMPORTS)

# STBLIT Test
ifeq "$(ENABLE_STBLIT_MACROS)" "TRUE"
    IMPORTS += stblit
endif

ifeq "$(STVID_USE_BLIT)" "TRUE"
    IMPORTS += stblit
endif

ifeq "$(STVID_USE_STBLIT_TEST_MACROS)" "TRUE"
    IMPORTS += stblit
endif


# Sources of the components
# -------------------------

# Object for ST20
ST20_OBJS := vid_test.tco

ifneq (,$(findstring $(DVD_BACKEND),DELTAPHI_HE))
        ST20_OBJS += lx_loader.tco
endif

ifdef NATIVE_COMPIL
        ST20_OBJS += dllinit.tco
endif

ifeq "$(DVD_OS)" "LINUX"
# Object for LINUX
ST40_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
else
# Object for ST40
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
endif


# Add stmes library if compiling for secure 7109
ifeq "$(DVD_BACKEND)" "7109"
    ifneq ($(DVD_SECURED_CHIP),)
        ST40_OBJS += crypto.o crypto_mme_host.o
    endif
endif

ifdef MULTICOM
    CFLAGS := $(CFLAGS) -DMULTICOM -D__SH4__
    INCLUDE_PATH += $(RPC_ROOT)/include
    INCLUDE_PATH += $(RPC_ROOT)/src/mme/include
    INCLUDE_PATH += $(RPC_ROOT)/src/embx/include
	ifeq "$(DVD_TOOLSET)" "ST40WINCE"
		FULLDIR_EMBX := $(RPC_ROOT)/lib/wince/$(ARCHITECTURE)
	else
		FULLDIR_EMBX := $(RPC_ROOT)/lib/os21/$(ARCHITECTURE)
	endif
else
    ifeq "$(DVD_BUILDING_IN_VOB)" "false"
        INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/,mme/src/Embx/include))
        INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/,mme/src/Embx/src/embx/embxshm))
    else
        INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/*-prj-,mme/src/Embx/include))
        INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/*-prj-,mme/src/Embx/src/embx/embxshm))
    endif
endif

# Object for ST200
ST200_OBJS = $(ST40_OBJS)

# Additional components in sub-directories
ifndef VALID_TOOLS
ifneq "$(DVD_OS)" "LINUX"
VIDTEST_MODULES = dhrystone
endif
endif

ifeq ($(USE_HDD), TRUE)
     VIDTEST_MODULES += hdd
endif

ifeq ($(USE_OLD_HDD), TRUE)
    VIDTEST_MODULES += old_hdd
endif

ifeq ($(USE_FILESYSTEM), TRUE)
    VIDTEST_MODULES += hdd_filesystem
endif

# Flags and options
# -----------------
SUBDIRS += $(VIDTEST_MODULES)
STAPIGAT_INCLUDE_PATH := $(addprefix -I stvid/tests/src/,$(VIDTEST_MODULES))
ifeq "$(DVD_BACKEND)" "7200"
    STAPIGAT_INCLUDE_PATH += -I stpti4/src/stpti/frontend/stfe
endif
ifeq "$(DVD_OS)" "LINUX"
STAPIGAT_INCLUDE_PATH += stfdma stvid/tests
ifndef VALID_TOOLS
    STAPIGAT_INCLUDE_PATH += stcommon
endif
endif
# For compilation of pti+link or stptii
CFLAGS += $(OPTIONAL_CFLAGS)

STAPIGAT_FLAGS :=
STTBX_FLAGS :=

# For stapigat new api
ifneq (,$(findstring $(DVD_BACKEND),7200))
    STAPIGAT_FLAGS += -DSTAPIGAT_USE_NEW_INIT_API
    CFLAGS += -DSTAPIGAT_USE_NEW_INIT_API
endif

# for stavmem kernel mode
ifeq "$(DVD_OS)" "LINUX"
  CFLAGS += -DAVMEM_KERNEL_MODE
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

#disable clkrv use in stapigat
ifneq ($(STVID_NO_CLKRV),)
	STAPIGAT_FLAGS += -DSTCLKRV_UNAVAILABLE
endif

# enable use of vid_loadtill & still.h in stapigat
STAPIGAT_FLAGS += -DVIDEO_TEST_HARNESS_BUILD

ifeq "$(DVD_BACKEND)" "7710"
  ifeq "$(STI7710_CUT2x)" "TRUE"
     CFLAGS += -DSTI7710_CUT2x
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

# Use splitted scripts in the sscripts directory
ifneq ($(STVID_USE_SPLITTED_SCRIPTS),)
	CFLAGS += -DSTVID_USE_SPLITTED_SCRIPTS
	STAPIGAT_FLAGS += -DSTVID_USE_SPLITTED_SCRIPTS
endif

# For STAPIGAT, caches, injection workaround
#ifneq ($(DEBUG),)
#    ifneq "$(BUILDING_FOR)" "ST200"
#        STVID_ICACHE_DISABLED := 1
#    endif
#    	STVID_DCACHE_DISABLED := 1
#endif

ifeq ($(STVID_DCACHE_DISABLED),)
    STAPIGAT_FLAGS += -D DCACHE_ENABLE
endif
ifeq ($(STVID_ICACHE_DISABLED),)
    STAPIGAT_FLAGS += -D ICACHE_ENABLE
endif

ifdef STVID_7100_SOFTWARE_PLATFORM
    CFLAGS += -D STVID_7100_SOFTWARE_PLATFORM
    STAPIGAT_FLAGS += -D STVID_7100_SOFTWARE_PLATFORM
endif

ifdef STVID_7109_SOFTWARE_PLATFORM
    CFLAGS += -D STVID_7109_SOFTWARE_PLATFORM
    STAPIGAT_FLAGS += -D STVID_7109_SOFTWARE_PLATFORM
endif

ifdef STUB_VTG
    CFLAGS += -D STUB_VTG
    STAPIGAT_FLAGS += -D STUB_VTG
endif

ifdef VALID_TOOLS
    CFLAGS += -D VALID_TOOLS
    STAPIGAT_FLAGS += -D VALID_TOOLS
endif

ifdef STVID_VALID
    CFLAGS += -D STVID_VALID
    STAPIGAT_FLAGS += -D STVID_VALID
endif

ifdef VALID_SKIP
    STAPIGAT_FLAGS += -D VALID_SKIP
endif

ifdef STVID_NO_DISPLAY
    STAPIGAT_FLAGS += -D STVID_NO_DISPLAY
endif

ifneq ($(STVID_TRACE_BUFFER_UART),)
    CFLAGS += -D STVID_TRACE_BUFFER_UART=$(STVID_TRACE_BUFFER_UART)
    STAPIGAT_FLAGS += -D STVID_TRACE_BUFFER_UART=$(STVID_TRACE_BUFFER_UART)
endif

ifneq ($(STVID_DEBUG_GET_STATISTICS),)
    CFLAGS += -D STVID_DEBUG_GET_STATISTICS
    STAPIGAT_FLAGS += -D STVID_DEBUG_GET_STATISTICS
endif


ifneq ($(VIDEO_DEBUG_DEINTERLACING_MODE),)
    CFLAGS += -D VIDEO_DEBUG_DEINTERLACING_MODE
    STAPIGAT_FLAGS += -D VIDEO_DEBUG_DEINTERLACING_MODE
endif

ifneq ($(STVID_DEBUG_GET_DISPLAY_PARAMS),)
    CFLAGS += -D STVID_DEBUG_GET_DISPLAY_PARAMS
    STAPIGAT_FLAGS += -D STVID_DEBUG_GET_DISPLAY_PARAMS
endif



ifneq ($(STVID_DEBUG_GET_STATUS),)
    CFLAGS += -D STVID_DEBUG_GET_STATUS
    STAPIGAT_FLAGS += -D STVID_DEBUG_GET_STATUS
endif

ifneq ($(STLAYER_CONVERT_PALETTE_TO_YCBCR888),)
    CFLAGS += -D STLAYER_CONVERT_PALETTE_TO_YCBCR888
    STAPIGAT_FLAGS += -D STLAYER_CONVERT_PALETTE_TO_YCBCR888
endif

ifneq ($(STLAYER_CONVERT_PALETTE_TO_AYCBCR8888),)
    CFLAGS += -D STLAYER_CONVERT_PALETTE_TO_AYCBCR8888
    STAPIGAT_FLAGS += -D STLAYER_CONVERT_PALETTE_TO_AYCBCR8888
endif

ifeq "$(USE_215K_BLITTER_CACHE)" "TRUE"
    DVD_CFLAGS += -DUSE_215K_BLITTER_CACHE
else
        ifeq "$(USE_128K_BLITTER_CACHE)" "TRUE"
                DVD_CFLAGS += -DUSE_128K_BLITTER_CACHE
        else
                ifeq "$(USE_64K_BLITTER_CACHE)" "TRUE"
                        DVD_CFLAGS += -DUSE_64K_BLITTER_CACHE
                else
                        ifeq "$(USE_32K_BLITTER_CACHE)" "TRUE"
                                DVD_CFLAGS += -DUSE_32K_BLITTER_CACHE
                        endif
                endif
        endif
endif

ifneq ($(STVID_USE_REDUCED_SYSTEM_SIZE),)
    CFLAGS += -D STVID_USE_REDUCED_SYSTEM_SIZE
    STAPIGAT_FLAGS += -D STVID_USE_REDUCED_SYSTEM_SIZE
endif

ifneq ($(ENABLE_RECCURENCE_MODE_TEST),)
    CFLAGS += -D ENABLE_RECCURENCE_MODE_TEST
    STAPIGAT_FLAGS += -D ENABLE_RECCURENCE_MODE_TEST
endif

# Enable DEFINED_51xx flag for 51xx devices family.
 ifneq (,$(findstring "$(DVD_BACKEND)","5100" "5105" "5301" "5107" "5188" "5525" "5162"))
    CFLAGS += -D ST_51xx_Device
    STAPIGAT_FLAGS += -D ST_51xx_Device
 endif


# Add define to enable additional function to set the synchronization delay (like the open param)
ifneq ($(STVID_ENABLE_SYNCHRONIZATION_DELAY),)
    CFLAGS += -D STVID_ENABLE_SYNCHRONIZATION_DELAY
    STAPIGAT_FLAGS += -D STVID_ENABLE_SYNCHRONIZATION_DELAY
endif

ifneq ($(USE_HDD),)
    CFLAGS += -D USE_HDD
    STAPIGAT_FLAGS += -D USE_HDD
endif
ifneq ($(USE_OLD_HDD),)
    CFLAGS += -D USE_HDD
    STAPIGAT_FLAGS += -D USE_HDD
endif

ifneq ($(USE_TUNER),)
    CFLAGS += -D USE_TUNER
    STAPIGAT_FLAGS += -D USE_TUNER
endif

ifneq ($(USE_AUDIO),)
    CFLAGS += -D USE_AUDIO
    STAPIGAT_FLAGS += -D USE_AUDIO
endif

ifneq ($(USE_OSPLUS),)
    DVD_CFLAGS += -D USE_OSPLUS
    STAPIGAT_FLAGS += -D USE_OSPLUS
endif

ifneq ($(USE_FILESYSTEM),)
    DVD_CFLAGS += -D USE_FILESYSTEM
    STAPIGAT_FLAGS += -D USE_FILESYSTEM
endif

ifdef STVID_USE_FMD
    STAPIGAT_FLAGS += -D STVID_USE_FMD
    DVD_CFLAGS += -D STVID_USE_FMD
endif

ifdef STVID_USE_CRC
    STAPIGAT_FLAGS += -D STVID_USE_CRC -D STLAYER_USE_CRC
    DVD_CFLAGS += -D STVID_USE_CRC -D STLAYER_USE_CRC
endif

ifdef STVID_USE_DEBLOCK
    STAPIGAT_FLAGS += -D STVID_USE_DEBLOCK
    DVD_CFLAGS += -D STVID_USE_DEBLOCK
    ifneq ($(VIDEO_DEBLOCK_DEBUG),)
	STAPIGAT_FLAGS += -D VIDEO_DEBLOCK_DEBUG
	DVD_CFLAGS += -D VIDEO_DEBLOCK_DEBUG
    endif
endif

ifneq ($(STVID_WA_GNBvd62704),)
    CFLAGS += -D WA_GNBvd62704
    STAPIGAT_FLAGS += -D WA_GNBvd62704
endif

# Add define for test application to be informed of harware error
ifneq ($(STVID_HARDWARE_ERROR_EVENT),)
    CFLAGS += -D STVID_HARDWARE_ERROR_EVENT
    STAPIGAT_FLAGS += -D STVID_HARDWARE_ERROR_EVENT
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
        # For DVB, use STPTI4_DVB-REL_xxx ; for DirecTV, use STPTI4_DTV-REL_xxx
        ifeq ($(DVD_SERVICE),DIRECTV)
            CFLAGS += -D STPTI_DTV_SUPPORT -D STVID_DIRECTV
            STAPIGAT_FLAGS += -D STPTI_DTV_SUPPORT -D STVID_DIRECTV -D STMERGE_DTV_PACKET
            STPTI_OPTIONS += -D STPTI_DTV_SUPPORT
            STCLKRV_OPTIONS += -D STPTI_DTV_SUPPORT
            STMERGE_DTV_PACKET=1
            export STMERGE_DTV_PACKET
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

ifeq "$(DVD_TRANSPORT)" "demux"
    ifneq (,$(findstring $(DVD_FRONTEND), 5188))
        CFLAGS += -D STDEMUX_FDMA_SUPPORT
        STAPIGAT_FLAGS += -D STDEMUX_FDMA_SUPPORT
        STDEMUX_OPTIONS += -DSTDEMUX_SINGLE_INPUT_BUFFER -DSTDEMUX_FDMA_SUPPORT -DSTDEMUX_USE_PID_TABLE=32
        STCLKRV_OPTIONS += -D STDEMUX_FDMA_SUPPORT
    else
        ifneq (,$(findstring $(DVD_FRONTEND), 8010))
           CFLAGS += -D STDEMUX_FLEXIDMA_SUPPORT
           STAPIGAT_FLAGS += -D STDEMUX_FLEXIDMA_SUPPORT
           STDEMUX_OPTIONS += -D STDEMUX_FLEXIDMA_SUPPORT
           STCLKRV_OPTIONS += -D STDEMUX_FLEXIDMA_SUPPORT
        endif
    endif
endif

# for Old STVIN
ifeq ($(DVD_STVIN),MASTER)
    CFLAGS += -D ST_MasterDigInput
    STAPIGAT_FLAGS += -D ST_MasterDigInput
endif

BACKEND_USE_CLKRV = 7710 7100 7109 7200
ifeq "$(findstring $(DVD_BACKEND), $(BACKEND_USE_CLKRV))" "$(DVD_BACKEND)"
  STAPIGAT_FLAGS += -DSTVTG_USE_CLKRV
endif

# STBLIT Test
ifeq "$(ENABLE_STBLIT_MACROS)" "TRUE"
  CFLAGS += -DENABLE_STBLIT_MACROS
  STAPIGAT_FLAGS += -DENABLE_STBLIT_MACROS
endif

ifeq "$(STVID_USE_BLIT)" "TRUE"
  CFLAGS += -D STVID_USE_BLIT
 endif

ifeq "$(STVID_USE_STBLIT_TEST_MACROS)" "TRUE"
    STAPIGAT_FLAGS += -DSTVID_USE_STBLIT_TEST_MACROS
endif

# Specific mb290 option
ifeq "$(DVD_PLATFORM)" "mb290"
    STCOMMON_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
    STPTI_OPTIONS += -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
endif

ifneq ($(USE_STEM7020),)
    STBOOT_OPTIONS := -D STBOOT_DB573_PCF8575_ADDRESS=0x42
    ifeq "$(DVD_FRONTEND)" "5514"
        STCOMMON_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
        STPTI_OPTIONS += -DREMOVE_GENERIC_ADDRESSES -DUSE_5514_GENERIC_ADDRESSES
    endif
    ifeq "$(DVD_FRONTEND)" "5517"
        STCOMMON_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_5517_GENERIC_ADDRESSES
        STPTI_OPTIONS += -DREMOVE_GENERIC_ADDRESSES -DUSE_5517_GENERIC_ADDRESSES
    endif
endif

ifneq ($(USE_DELTAPHI_HE),)
    STVID_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_DELTAPHI_HE_GENERIC_ADDRESSES
    STFDMA_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_DELTAPHI_HE_GENERIC_ADDRESSES
    MME_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_DELTAPHI_HE_GENERIC_ADDRESSES
endif

ifneq ($(USE_DELTAMU_HE),)
    STVID_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_DELTAPHI_HE_GENERIC_ADDRESSES
    STFDMA_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_DELTAPHI_HE_GENERIC_ADDRESSES
    MME_OPTIONS := -DREMOVE_GENERIC_ADDRESSES -DUSE_DELTAPHI_HE_GENERIC_ADDRESSES
endif

ifneq ($(STPTI_5_1_6),)
    STAPIGAT_FLAGS += -DSTPTI_5_1_6
endif

# add stvin commands for mb295 and mb290 only
ifneq (,$(findstring $(DVD_PLATFORM),mb295))
    STVIN_OPTIONS += -D STVIN_WA_SAA7114
endif
ifneq (,$(findstring $(DVD_PLATFORM),mb290))
    STVIN_OPTIONS += -D STVIN_WA_SAA7114
endif

# Test harness only related flag. By default, PBO is disabled in test harness.
# if STVID_ENABLE_PBO environment variable is defined then PBO is enabled, data
# injection form memory are then monitored by bit buffer level but injection
# from live becomes not reliable. This mode (STVID_ENABLE_PBO=1) must only be set
# for debug investigation purposes
ifeq ($(STVID_ENABLE_PBO),1)
    STAPIGAT_FLAGS += -DSTVID_VIDDEC_PBO_ENABLED
endif

ifeq "$(COMPO_NO_STATIC_BASE_ADDRESS)" "TRUE"
    COMPO_FLAGS += -DDONT_USE_STATIC_BASE_ADDRESS
endif

# multicom transport configuration
ifeq "$(DVD_BACKEND)" "DELTAPHI_HE"
    export MULTICOM_COMPANION_CONFIG=0x03
endif

ifeq "$(DVD_BACKEND)" "DELTAMU_HE"
    export MULTICOM_COMPANION_CONFIG=0x03
endif

ifeq "$(DVD_BACKEND)" "7100"
    ifndef MULTICOM_COMPANION_CONFIG
        export MULTICOM_COMPANION_CONFIG=0x07
    endif
endif

ifeq "$(DVD_BACKEND)" "7109"
    ifndef MULTICOM_COMPANION_CONFIG
        export MULTICOM_COMPANION_CONFIG=0x07
    endif
endif

ifeq "$(DVD_BACKEND)" "7200"
# Possible values of MULTICOM_COMPANION_CONFIG
# 0x01 => ST40 (mandatory)
# 0x02 => LX_AUDIO1
# 0x04 => LX_VIDEO1
# 0x08 => LX_AUDIO2
# 0x10 => LX_VIDEO2
#
# Exemples:
# 0x05 => ST40 + LX_VIDEO1
# 0x15 => ST40 + LX_VIDEO1 + LX_VIDEO2
# 0x07 => ST40 + LX_VIDEO1 + LX_AUDIO1

    ifndef MULTICOM_COMPANION_CONFIG
        export MULTICOM_COMPANION_CONFIG=0x15
    endif
endif

ifeq "$(DVD_BACKEND)" "ZEUS"
    ifndef MULTICOM_COMPANION_CONFIG
        export MULTICOM_COMPANION_CONFIG=0x07
    endif
endif

ifdef MULTICOM_COMPANION_CONFIG
    CFLAGS += -DMULTICOM_COMPANION_CONFIG=$(MULTICOM_COMPANION_CONFIG)
    STAPIGAT_FLAGS += -DMULTICOM_COMPANION_CONFIG=$(MULTICOM_COMPANION_CONFIG)
endif

ifneq ($(STVID_ENABLE_PREPROCESSING_TEST),)
    CFLAGS += -D STVID_ENABLE_PREPROCESSING_TEST
    STAPIGAT_FLAGS += -D STVID_ENABLE_PREPROCESSING_TEST
endif


# Specific compilation flags
stapigat.lib libstapigat.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STAPIGAT_FLAGS) -DSTTBX_PRINT -DSTTBX_INPUT
	STAPIGAT_INCLUDE := stvid/tests/src $(STAPIGAT_INCLUDE_PATH)
	STAPIGAT_IMPORTS_ST20 := $(IMPORTS)
	ifndef VALID_TOOLS
		STAPIGAT_IMPORTS_ST20 += dumpreg
	endif
    # add stextvin commands for mb295, mb290,mb391,mb411 and STEM7020 only
	ifneq (,$(findstring $(DVD_PLATFORM),mb411))
		STAPIGAT_IMPORTS_ST20 += stextvin
	endif
	ifneq (,$(findstring $(DVD_PLATFORM),mb295))
		STAPIGAT_IMPORTS_ST20 += stextvin
	endif
	ifneq (,$(findstring $(DVD_PLATFORM),mb290))
		STAPIGAT_IMPORTS_ST20 += stextvin
	endif
    ifneq (,$(findstring $(DVD_PLATFORM),mb391))
		STAPIGAT_IMPORTS_ST20 += stextvin
	endif
    ifneq ($(USE_STEM7020),)
		STAPIGAT_IMPORTS_ST20 += stextvin
	endif
	STAPIGAT_IMPORTS_ST40  := $(STAPIGAT_IMPORTS_ST20)
	STAPIGAT_IMPORTS_ST200 := $(STAPIGAT_IMPORTS_ST40)

export STAPIGAT_INCLUDE
export STAPIGAT_IMPORTS_ST20
export STAPIGAT_IMPORTS_ST40
export STAPIGAT_IMPORTS_ST200

staud.lib libstaud.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTI2C_SW_START_STOP

 # STAVMEM is special in linux - no library is created so we must not add
 # the library to the link command. The following link option suppresses this.
 # This only works currently on LINUX.

LINUX_libstavmem.a_NOLINK := 1

ifeq "$(DVD_PLATFORM)" "mb290"
# To allow show/hide test with stvid_duplicatepicture on atv2 :
# avmem should use memcpy() method instead of 2Dblockmove or gpdma
stavmem.lib libstavmem.a :\
        STAVMEM_NO_2DBM=1
        STAVMEM_NO_GPDMA=1
export STAVMEM_NO_2DBM
export STAVMEM_NO_GPDMA
endif

ifeq "$(DVD_BACKEND)" "DELTAPHI_HE"
# To allow show/hide test with stvid_duplicatepicture on atv2 :
# avmem should use memcpy() method instead of 2Dblockmove or gpdma
stavmem.lib libstavmem.a :\
        STAVMEM_NO_2DBM=1
        STAVMEM_NO_GPDMA=1
export STAVMEM_NO_2DBM
export STAVMEM_NO_GPDMA
endif

ifneq (,$(findstring $(DVD_BACKEND), 7100 7109 7200))
# Force max STAVMEM partition number for 7100 & 7109 chips
# One more partition for Security features if DVD_SECURED_CHIP is set.
ifneq ($(DVD_SECURED_CHIP),)
stavmem.lib libstavmem.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTAVMEM_MAX_MAX_PARTITION=5
else
stavmem.lib libstavmem.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTAVMEM_MAX_MAX_PARTITION=4
endif
endif

ifeq "$(DVD_BACKEND)" "DELTAMU_HE"
# To allow show/hide test with stvid_duplicatepicture on atv2 :
# avmem should use memcpy() method instead of 2Dblockmove or gpdma
stavmem.lib libstavmem.a :\
        STAVMEM_NO_2DBM=1
        STAVMEM_NO_GPDMA=1
export STAVMEM_NO_2DBM
export STAVMEM_NO_GPDMA
endif

stblit.lib libstblit.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTBLIT_MASTER_TASK_STACK_PRIORITY=4 -DSTBLIT_SLAVE_TASK_STACK_PRIORITY=4 -DSTBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY=5

 # STFDMA is special in linux - no library is created so we must not add
 # the library to the link command. The following link option suppresses this.
 # This only works currently on LINUX.

LINUX_libstfdma.a_NOLINK := 1

# Removed setting of ST40_NOCACHE_NOTRANSLATE compile flag that conflicts with macro in sti7100h and sti7109.h
ifneq (,$(findstring $(DVD_BACKEND), 7100 7109 7200))
# Do not use ST40_NOCACHE_NOTRANSLATE flag to compile libstfdma.a
else
stfdma.lib libstfdma.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DST40_NOCACHE_NOTRANSLATE
endif

stboot.lib libstboot.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STBOOT_OPTIONS)

ifneq ($(STVID_NO_PTI),)
stclkrv.lib libstclkrv.a :\
        STCLKRV_NO_PTI := 1
        DVD_CFLAGS := $(DVD_CFLAGS) $(STCLKRV_OPTIONS)
export STCLKRV_NO_PTI
else
stclkrv.lib libstclkrv.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STCLKRV_OPTIONS)
endif

stcommon.lib libstcommon.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STCOMMON_OPTIONS)

stlayer.lib libstlayer.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_GAMMAGFX_TASK_PRIORITY=7 $(STLAYER_OPTIONS)

$(DVD_TRANSPORT).lib lib$(DVD_TRANSPORT).a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STPTI_OPTIONS)
        STPTI_CONFIG := $(STPTI_OPTIONS)
        STDEMUX_CONFIG := $(STDEMUX_OPTIONS)

sttbx.lib libsttbx.a : \
        DVD_CFLAGS := $(DVD_CFLAGS) $(STTBX_FLAGS)

stuart.lib libstuart.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTUART_TIMER_TASK_PRIORITY=3

sttuner.lib libsttuner.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTTBX_PRINT -DSTTBX_INPUT

compo.lib libcompo.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(COMPO_FLAGS)

# Priority for video is Display > Trickmode > Decode > Error Recovery
stvid.lib libstvid.a :\
	DVD_CFLAGS := $(DVD_CFLAGS) $(STVID_OPTIONS) -DSTVID_TASK_PRIORITY_DECODE=10 -DSTVID_TASK_PRIORITY_DISPLAY=12 -DSTVID_TASK_PRIORITY_ERROR_RECOVERY=9 -DSTVID_TASK_PRIORITY_TRICKMODE=11  -DSTVID_TASK_PRIORITY_INJECTERS=9

# add stvin commands for some boards only
stvin.lib libstvin.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) $(STVIN_OPTIONS)

stvtg.lib libstvtg.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTVTG_TASK_PRIORITY_IT_PROCESSING=15 -DSTVTG_NOTIFY_VSYNC_UNDER_IT

valitls.lib libvalitls.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -DSTAPI_APPLICATION

# temporary workaround to avoid compiling problems
ifeq "$(DVD_FRONTEND)" "5528"
stclkrv.lib libstclkrv.a stgpdma.lib libstgpdma.a:\
        DVD_CFLAGS := $(DVD_CFLAGS) -I $(wildcard $(DVD_ROOT)/*stos)
endif

ifeq (,$(findstring $(DVD_BACKEND), 5100 5162))
statapi.lib libstatapi.a :\
        DVD_CFLAGS := $(DVD_CFLAGS) -I $(wildcard $(DVD_ROOT)/*stos)
endif

ifeq "$(DVD_OS)" "LINUX"
DVD_CFLAGS := $(DVD_CFLAGS) -D$(LINUX_ARCHITECTURE)
endif


# DVD_CFLAGS would be exported for recursive makefile (not CFLAGS)
export DVD_CFLAGS
export STPTI_CONFIG
export STDEMUX_CONFIG

# For output of toolbox
CFLAGS += -DSTTBX_PRINT -DSTTBX_INPUT

ifeq ($(FLOATING_POINT), fp)
        LKFLAGS += -fp
endif

# Warning messages
MESSAGE1=.
ifeq "$(DVD_FRONTEND)" "5518"
	MESSAGE1=Compilation with DVD_FRONTEND_CPUCLOCK=$(DVD_FRONTEND_CPUCLOCK)
	ifneq ($(DVD_FRONTEND_CPUCLOCK),81)
		MESSAGE1=!!! Warning : DVD_FRONTEND_CPUCLOCK should be 81 !!!
	endif
endif

MESSAGE2=Compilation with $(DVD_TRANSPORT)
ifeq ($(DVD_FRONTEND), 5518)
	ifeq ($(DVD_TRANSPORT),pti)
		MESSAGE2=!!! Warning : LINK should be used !!!
	else
		ifeq ($(DVD_TRANSPORT),stpti)
			MESSAGE2=!!! Warning : LINK should be used !!!
		endif
	endif
else
	ifeq ($(DVD_FRONTEND), 5508)
		ifeq ($(DVD_TRANSPORT),pti)
			MESSAGE2=!!! Warning : LINK should be used !!!
		else
			ifeq ($(DVD_TRANSPORT),stpti)
				MESSAGE2=!!! Warning : LINK should be used !!!
			endif
		endif
	else
		ifeq ($(DVD_TRANSPORT),link)
			MESSAGE2=!!! Warning : Bad transport choice, STPTI or PTI should be used !!!
		endif
	endif
endif

# Target
# ------

ST20_TARGETS = vid_test.lku
ifeq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20EXE_TO_LINUXEXE,$(ST20_TARGETS))
else
ST40_TARGETS = $(call ST20EXE_TO_ST40EXE,$(ST20_TARGETS))
endif
ST200_TARGETS = $(ST40_TARGETS)

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build general application for DRAM
vid_test_INIT := board_init

BOARDSPECS=
ifneq (,$(findstring $(DVD_BACKEND),7100 7109 7200 DELTAPHI_HE DELTAMU_HE))
ifeq "$(DVD_OS)" "LINUX"
IMPORT_EMBX_LIBS =
IMPORT_STOSPLUS_LIBS =
IMPORT_STOSPLUS_REGISTRY_LIBS =
else
# FULLDIR_EMBX = $(RPC_ROOT)/lib/os21/$(BUILDING_FOR)
# !! WARNING !! Can't use $(BUILDING_FOR) in FULLDIR_EMBX because architecture is
# upper case and MULTICOM directories are lower case -> issue for use under Unix

ifeq "$(BUILDING_FOR)" "ST40"
    ifeq "$(DVD_TOOLSET)" "ST40WINCE"
        FULLDIR_EMBX := $(RPC_ROOT)/lib/wince/st40
    else
        FULLDIR_EMBX := $(RPC_ROOT)/lib/os21/st40
    endif
    ifeq (,$(findstring $(DVD_BACKEND),DELTAPHI_HE DELTAMU_HE))
        ifeq "$(DVD_BACKEND)" "7109"
            ifneq ($(DVD_SECURED_CHIP),)
                BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7109_secure
            else
                BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7109
            endif
        else
            ifeq "$(DVD_BACKEND)" "7200"
               export USE_MBOARD=board_$(DVD_PLATFORM)_lmi0_$(MULTICOM_COMPANION_CONFIG)

               BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs_7200
            else
               BOARDSPECS=-specs=$(DVD_ROOT)/dvdgr-prj-stapigat/src/boardspecs
            endif
        endif
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

ifeq "$(DVD_TOOLSET)" "ST40WINCE"
  IMPORT_EMBX_LIBS = embxmailbox.lib embxshm.lib embxshell.lib mme_host.lib
  $(IMPORT_EMBX_LIBS):
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH), $(FULLDIR_EMBX)/$(LIBNAME_FROM_AT_F)) \
		$(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_$(BUILDING_FOR)_EXPORTS)) > $(NULL)
else
EMBX_IMPORTS = embxmailbox embxshm embxshell mme_host
IMPORT_EMBX_LIBS = $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(EMBX_IMPORTS)))
$(IMPORT_EMBX_LIBS):
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH), $(FULLDIR_EMBX)/$(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(LIBNAME_FROM_AT_F)))) \
              $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_$(BUILDING_FOR)_EXPORTS)) > $(NULL)
endif

ifeq "$(USE_OSPLUS)" "TRUE"
    STOSPLUS_IMPORTS =
    FULLDIR_STOSPLUS := $(OSPLUS_ROOT)/lib/st40/
    STOSPLUS_IMPORTS += ospluslib usblink

    IMPORT_STOSPLUS_LIBS = $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(STOSPLUS_IMPORTS)))

$(IMPORT_STOSPLUS_LIBS):
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH), $(FULLDIR_STOSPLUS)/$(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(LIBNAME_FROM_AT_F)))) \
              $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_$(BUILDING_FOR)_EXPORTS)) > $(NULL)

    OSPLUS_REG_BOARD =
    ifeq "$(DVD_PLATFORM)" "mb411"
        ifeq "$(DVD_BACKEND)" "7100"
            OSPLUS_REG_BOARD=mb411stb7100
        endif
        ifeq "$(DVD_BACKEND)" "7109"
            OSPLUS_REG_BOARD=mb411stb7109
        endif
    endif
    ifeq "$(DVD_PLATFORM)" "mb442"
        ifeq "$(DVD_BACKEND)" "7100"
            OSPLUS_REG_BOARD=mb442stb7100
        endif
        ifeq "$(DVD_BACKEND)" "7109"
            OSPLUS_REG_BOARD=mb442stb7109
        endif
    endif
    ifeq "$(DVD_PLATFORM)" "mb519"
        OSPLUS_REG_BOARD=$(DVD_PLATFORM)
    endif

    FULLDIR_STOSPLUS_REGISTRY := $(OSPLUS_ROOT)/lib/st40/$(OSPLUS_REG_BOARD)
    STOSPLUS_IMPORTS_REGISTRY = registry
    IMPORT_STOSPLUS_REGISTRY_LIBS = $(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(STOSPLUS_IMPORTS_REGISTRY)))

$(IMPORT_STOSPLUS_REGISTRY_LIBS):
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH), $(FULLDIR_STOSPLUS_REGISTRY)/$(addprefix $(LIB_PREFIX),$(addsuffix $(LIB_SUFFIX),$(LIBNAME_FROM_AT_F)))) \
              $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_$(ARCHITECTURE)_EXPORTS)) > $(NULL)
else
    IMPORT_STOSPLUS_LIBS =
    IMPORT_STOSPLUS_REGISTRY_LIBS =
endif

endif
endif


BACKEND_USE_MULTICOM = 7100 7109 7200 DELTAPHI_HE DELTAMU_HE
ifeq "$(findstring $(DVD_BACKEND), $(BACKEND_USE_MULTICOM))" "$(DVD_BACKEND)"
$($(BUILDING_FOR)_TARGETS): $(IMPORT_LIBS) $($(BUILDING_FOR)_OBJS) $(SUBDIR_LIBS) $(IMPORT_EMBX_LIBS) $(IMPORT_STOSPLUS_LIBS) $(IMPORT_STOSPLUS_REGISTRY_LIBS)
else
$($(BUILDING_FOR)_TARGETS): $(IMPORT_LIBS) $($(BUILDING_FOR)_OBJS) $(SUBDIR_LIBS) $(IMPORT_STOSPLUS_LIBS) $(IMPORT_STOSPLUS_REGISTRY_LIBS)
endif
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	@$(ECHO) Linking $@
ifeq "$(BUILDING_FOR)" "ST20"
	$(LINK_EXECUTABLE) -M ../../vid_test.map
endif

ifeq "$(BUILDING_FOR)" "ST40"
ifeq "$(DVD_OS)" "LINUX"
	$(LINK_EXECUTABLE)
	$(CP) $(ST40_TARGETS) $(DVD_EXPORTS)/bin/$(ST40_TARGETS).exe
else
    ifeq "$(DVD_TOOLSET)" "ST40WINCE"
	$(LINK_EXECUTABLE) /MAP:../../vid_test.map $(BOARDSPECS)
else
	$(LINK_EXECUTABLE) -Xlinker -Map=../../vid_test.map $(BOARDSPECS)
endif
endif
endif

ifeq "$(BUILDING_FOR)" "ST200"
	$(LINK_EXECUTABLE)
endif

# Dependencies for crypto files
ifeq "$(DVD_BACKEND)" "7109"
ifneq ($(DVD_SECURED_CHIP),)
crypto.o crypto_mme_host.o : crypto.h \
                             crypto_mme.h \
                             CryptoAPI.h \
                             keys.h
endif
endif

# Build video driver and test application but ignore dependencies
ifeq "$(BUILDING_FOR)" "ST20"
    VIDEOLIB = stvid.lib
endif
ifeq "$(BUILDING_FOR)" "ST40"
    VIDEOLIB = libstvid.a
endif
ifeq "$(BUILDING_FOR)" "ST200"
    VIDEOLIB = libstvid.a
endif

imports:
	@$(ECHO) IMPORTS: $(IMPORTS)
	@$(ECHO) ST200_IMPORTS: $(ST200_IMPORTS)
	@$(ECHO) ST40_IMPORTS: $(ST40_IMPORTS)
	@$(ECHO) IMPORT_LIBS: $(IMPORT_LIBS)
	@$(ECHO) STAPIGAT_IMPORTS_ST40: $(STAPIGAT_IMPORTS_ST40)
	@$(ECHO) IMPORT_EMBX_LIBS: $(IMPORT_EMBX_LIBS)
	@$(ECHO) IMPORT_STOSPLUS_LIBS: $(IMPORT_STOSPLUS_LIBS)
	@$(ECHO) IMPORT_STOSPLUS_REGISTRY_LIBS: $(IMPORT_STOSPLUS_REGISTRY_LIBS)

no_imports: $(VIDEOLIB) $($(BUILDING_FOR)_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) $(MESSAGE1)
	@$(ECHO) $(MESSAGE2)
	$(CC) $(LIBRARIES) -T $(DVD_PLATFORM).cfg \
        $($(BUILDING_FOR)_LKFLAGS) $(vid_test_LKFLAGS) $(vid_test_$(BUILDING_FOR)_LKFLAGS) \
        $^ $(IMPORT_LIBS) $(OSLIBS) -p $(vid_test_INIT) -o $($(BUILDING_FOR)_TARGETS)

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS) $($(BUILDING_FOR)_TARGETS_OTHER)
	-$(RM) ../../vid_test.map
ifeq "$(BUILDING_FOR)" "ST20"
	-$(RM) $(subst .lku,.dbg,$($(BUILDING_FOR)_TARGETS))
	-$(RM) $(subst .lku,.map,$($(BUILDING_FOR)_TARGETS))
endif

else

include $(DVD_MAKE)/builddir.mak

endif

ifndef STUB_FMW_ST40
  ifdef STUB_FMW_LX
    DEPEND_FMW_LX :=
    DEPEND_FMW_LX_CLEAN :=
  else
    DEPEND_FMW_LX := $(MAKE) -C $(DVD_ROOT)/dvdgr-prj-deltaphi/src/firmware/make
    DEPEND_FMW_LX_CLEAN := $(MAKE) -C $(DVD_ROOT)/dvdgr-prj-deltaphi/src/firmware/make clean
  endif

lx :
	$(MAKE) -C $(DVD_ROOT)/stbgr-prj-mme/src/Embx
	$(MAKE) -C $(DVD_ROOT)/stbgr-prj-mme/src/Embx install
	$(DEPEND_FMW_LX)
	$(MAKE) -C $(DVD_ROOT)/stbgr-prj-mme/src/MMEplus/Source/EMBX/Companion/Make/STm8000

clean_lx:
	$(MAKE) -C $(DVD_ROOT)/stbgr-prj-mme/src/Embx clean
	$(DEPEND_FMW_LX_CLEAN)
	$(MAKE) -C $(DVD_ROOT)/stbgr-prj-mme/src/MMEplus/Source/EMBX/Companion/Make/STm8000 clean

endif

