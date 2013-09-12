############
# stvid.lib
############

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# video specific variables
VID_MODULES = api buffers

# internal modules optional
VID_OPTIONAL_MODULES =
# Add display module by default, unless specified differently
ifeq ($(STVID_NO_DISPLAY),)
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) display
endif
# Add avsync module by default if there is display module, unless specified differently
ifneq ($(findstring display,$(VID_OPTIONAL_MODULES)),)
  ifeq ($(STVID_NO_AVSYNC),)
    VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) avsync
  endif
endif
# Add trickmode module by default if there is display module, unless specified differently
ifneq ($(findstring display,$(VID_OPTIONAL_MODULES)),)
  ifeq ($(STVID_NO_TRICKMODE),)
    ifneq "$(DVD_FRONTEND)" "7200"
    VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) trickmod
    endif
    ifneq ($(findstring $(DVD_BACKEND),7100 7109 7200),)
      VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) speed
        # Add define to be able to compile or not code for backward trickmodes
        ifeq ($(STVID_TRICKMODE_NO_BACKWARD),)
        VID_DEFS := $(VID_DEFS) -D STVID_TRICKMODE_BACKWARD
        ##yxl 2009-04-02 temp add MACRO STVID_USE_CRC for test DMBT weak signal
		VID_DEFS := $(VID_DEFS) -D STVID_USE_CRC 
        endif
    endif
  endif
endif

# Add STLAYER FMD flag if STVID_USE_FMD is defined
ifdef STVID_USE_FMD
  DVD_CFLAGS += -D STLAYER_USE_FMD
endif

# Add crc module if STVID_USE_CRC is defined
ifdef STVID_USE_CRC
  VID_OPTIONAL_MODULES += crc
endif

# Add deblocking module if STVID_USE_DEBLOCK is defined
ifdef STVID_USE_DEBLOCK
  VID_OPTIONAL_MODULES += deblock
	ifneq ($(VIDEO_DEBLOCK_DEBUG),)
	    DVD_CFLAGS += -D VIDEO_DEBLOCK_DEBUG
	endif
endif

VID_OPTIONAL_FEATURES =
# Add composition feature by default if there is display module, unless specified differently
ifneq ($(findstring display,$(VID_OPTIONAL_MODULES)),)
  ifeq ($(STVID_NO_COMPOSITION),)
    VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) composition
  endif
endif
ifeq ($(findstring composition,$(VID_OPTIONAL_FEATURES)),)
  VID_DEFS := $(VID_DEFS) -D STVID_NO_COMPOSITION
endif
# Add smooth backward feature by default if there is trickmode module, unless specified differently
ifneq ($(findstring trickmod,$(VID_OPTIONAL_MODULES)),)
  ifeq ($(STVID_NO_SMOOTH_BACKWARD),)
    VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) smoothbackward
  endif
endif

# Uncomment lines below to compile for debug in all modules (debug info)
# VID_DEFS := $(VID_DEFS) -D STTBX_REPORT

# Add define to get statistic information from the video driver.
ifneq ($(STVID_DEBUG_GET_STATISTICS),)
VID_DEFS := $(VID_DEFS) -D STVID_DEBUG_GET_STATISTICS
endif

# Add define to get access to API function related to deinterlacer.
ifneq ($(VIDEO_DEBUG_DEINTERLACING_MODE),)
VID_DEFS := $(VID_DEFS) -D VIDEO_DEBUG_DEINTERLACING_MODE
endif

# Add define to get access to API function related to deinterlacer.
ifneq ($(STVID_DEBUG_GET_DISPLAY_PARAMS),)
VID_DEFS := $(VID_DEFS) -D STVID_DEBUG_GET_DISPLAY_PARAMS
endif


# Add define to get status information from the video driver.
ifneq ($(STVID_DEBUG_GET_STATUS),)
VID_DEFS := $(VID_DEFS) -D STVID_DEBUG_GET_STATUS
endif

# Add define to get traces of picture's life
ifneq ($(STVID_TRACE_PICTURE_LIFE),)
VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) trace_picture_life
endif

# Add define to use validation specific code
ifneq ($(STVID_VALID),)
        VID_DEFS := $(VID_DEFS) -D STVID_VALID
endif

# Add define to use validation tools
ifneq ($(VALID_TOOLS),)
        VID_DEFS := $(VID_DEFS) -D VALID_TOOLS
endif

# Add define for injecter module stub
ifdef STUB_INJECT
VID_DEFS := $(VID_DEFS) -D STUB_INJECT
endif

ifdef MME_VIDEO_TRANSPORT_NAME
    VID_DEFS := $(VID_DEFS) -DMME_VIDEO_TRANSPORT_NAME=$(MME_VIDEO_TRANSPORT_NAME)
endif

# Add define to enable additional function to set the synchronisation delay (like the open param)
ifneq ($(STVID_ENABLE_SYNCHRONIZATION_DELAY),)
VID_DEFS := $(VID_DEFS) -D STVID_ENABLE_SYNCHRONIZATION_DELAY
endif

# Local objects which comprise this component
#---------------------------------------------
IMPORTS := sttbx stevt stvtg stlayer stgxobj stavmem stos
ifneq ($(findstring inject,$(VID_OPTIONAL_MODULES)),)
IMPORTS := $(IMPORTS) stfdma
endif
ifneq ($(findstring avsync,$(VID_OPTIONAL_MODULES)),)
IMPORTS := $(IMPORTS) stclkrv
endif
ifdef VALID_TOOLS
IMPORTS := $(IMPORTS) valitls
endif

ST20_EXPORTS := stvid.lib
ST20_TARGETS := stvid.lib
ST20_OBJS := vid_ctcm.tco
ifdef STVID_CODEC_ONLY
  ST20_OBJS =
endif

ST40_IMPORTS := $(IMPORTS)
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

ST200_IMPORTS := $(ST40_IMPORTS)
ST200_EXPORTS := $(ST40_EXPORTS)
ST200_TARGETS := $(ST40_TARGETS)
ST200_OBJS    := $(ST40_OBJS)

# DirectTV specific
ifeq ($(DVD_SERVICE),DIRECTV)
VID_DEFS := $(VID_DEFS) -D STVID_DIRECTV
endif

# To force compilation with both HAL's
#VID_HW_CELLS = omega1 omega2

# Define HW cells present
ifeq "$(DVD_BACKEND)" "5510"
  VID_HW_CELLS := $(VID_HW_CELLS) omega1
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
endif

ifeq "$(DVD_BACKEND)" "5512"
  VID_HW_CELLS := $(VID_HW_CELLS) omega1
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
endif

ifeq "$(DVD_BACKEND)" "5508"
  VID_HW_CELLS := $(VID_HW_CELLS) omega1
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
endif

ifeq "$(DVD_BACKEND)" "5518"
  VID_HW_CELLS := $(VID_HW_CELLS) omega1
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
    VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
endif

ifeq "$(DVD_BACKEND)" "5514"
  VID_HW_CELLS := $(VID_HW_CELLS) omega1
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
endif

ifeq "$(DVD_BACKEND)" "5516"
  VID_HW_CELLS := $(VID_HW_CELLS) omega1
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
    VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
endif

ifeq "$(DVD_BACKEND)" "5517"
  VID_HW_CELLS := $(VID_HW_CELLS) omega1
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
endif

ifeq "$(DVD_BACKEND)" "7015"
  VID_HW_CELLS := $(VID_HW_CELLS) omega2
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
  ifneq ($(findstring decode,$(VID_OPTIONAL_MODULES)),)
    # This chip needs a SW controller for multi-decode
    # VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) multidecodeswcontroller
  endif
  # Add define for STi7015 workaround needing application to stop injection
  VID_DEFS := $(VID_DEFS) -D STVID_INJECTION_BREAK_WORKAROUND
  # for new STVIN > 1.0.0A4, add DVD_STVIN="MASTER"
  ifeq ($(DVD_STVIN),MASTER)
    VID_OPTIONAL_FEATURES += MasterDigInput diginput
  else
    VID_OPTIONAL_MODULES += diginput
  endif
endif

ifeq "$(DVD_BACKEND)" "7020"
ifeq "$(DVD_FRONTEND)" "5528"
  VID_HW_CELLS := $(VID_HW_CELLS) sdmpgo2 sddispo2 genbuff
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
endif
  VID_HW_CELLS := $(VID_HW_CELLS) omega2
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
    VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
  ifneq ($(findstring decode,$(VID_OPTIONAL_MODULES)),)
    # This chip needs a SW controller for multi-decode
    # VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) multidecodeswcontroller
  endif
  # Add define for STi7020 workaround needing application to stop injection
  # Only available for Cut 1.0. No more for cut 2.0
  # VID_DEFS := $(VID_DEFS) -D STVID_INJECTION_BREAK_WORKAROUND
  # for new STVIN > 1.0.0A4, add DVD_STVIN="MASTER"
  ifeq ($(DVD_STVIN),MASTER)
    VID_OPTIONAL_FEATURES += MasterDigInput diginput
  else
    VID_OPTIONAL_MODULES += diginput
  endif
endif

ifeq "$(DVD_BACKEND)" "GX1"
  VID_HW_CELLS := $(VID_HW_CELLS) omega2
  # Remove avsync on GX1 platform
  VID_OPTIONAL_MODULES := $(subst avsync,,$(VID_OPTIONAL_MODULES))
  # for new STVIN > 1.0.0A4, add DVD_STVIN="MASTER"
  ifeq ($(DVD_STVIN),MASTER)
    VID_OPTIONAL_FEATURES += MasterDigInput diginput
  else
    VID_OPTIONAL_MODULES += diginput
  endif
endif

ifeq "$(DVD_BACKEND)" "5528"
  VID_HW_CELLS := $(VID_HW_CELLS) sdmpgo2 sddispo2 genbuff
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
  ifneq ($(findstring STVID_SECOND_INSTANCE_SHARING_THE_SAME_DECODER,$(DVD_CFLAGS)),)
    VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) SecondInstanceSharingTheSameDecoder
  endif
  # for new STVIN > 1.0.0A4, add DVD_STVIN=MASTER
  ifeq ($(DVD_STVIN),MASTER)
    VID_OPTIONAL_FEATURES += MasterDigInput diginput
  endif
endif

ifneq (,$(findstring "$(DVD_BACKEND)","5100" "5105" "5107" "5301" "5525" "5188" "5162"))
  VID_HW_CELLS := $(VID_HW_CELLS) lcmpegx1 blitdisp genbuff
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue inject
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec

  # Enable DEFINED_51xx flag for 51xx devices family.
  VID_DEFS := $(VID_DEFS) -D ST_51xx_Device
endif

ifeq "$(DVD_BACKEND)" "7710"
  VID_HW_CELLS := $(VID_HW_CELLS) lcmpegx1 sddispo2 genbuff
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode ordqueue inject
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) oldmpeg2codec
  # for new STVIN > 1.0.0A4, add DVD_STVIN="MASTER"
  ifeq ($(DVD_STVIN),MASTER)
    VID_OPTIONAL_FEATURES += MasterDigInput diginput
  else
    VID_OPTIONAL_MODULES += diginput
  endif
endif

ifeq "$(DVD_BACKEND)" "7100"
  VID_HW_CELLS := $(VID_HW_CELLS) lcmpegx1 sddispo2 genbuff
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode inject producer ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) multicodec oldmpeg2codec
  ifdef NATIVE_COMPIL
    VID_OPTIONAL_VIDCODECS := vcodh264 vcodmpg4p2 vcodvc1  #vc1 used for VSOC platform only
  else
    VID_OPTIONAL_VIDCODECS := vcodh264 vcodmpg4p2
  endif
  VID_DEFS := $(VID_DEFS) -D FORCE_SECONDARY_ON_AUX_DISPLAY
  # for new STVIN > 1.0.0A4, add DVD_STVIN="MASTER"
  ifeq ($(DVD_STVIN),MASTER)
    VID_OPTIONAL_FEATURES += MasterDigInput diginput
  else
    VID_OPTIONAL_MODULES += diginput
  endif
endif

ifeq "$(DVD_BACKEND)" "7109"
  VID_HW_CELLS := $(VID_HW_CELLS) lcmpegx1 sddispo2 genbuff displaypipe graphics
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode inject producer ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) multicodec oldmpeg2codec multifieldpresentation
  VID_OPTIONAL_VIDCODECS := vcodh264 vcodmpg4p2 vcodvc1 vcodavs
  ifneq ($(STVID_USE_ENCODER),)
#  ifeq ($(findstring STVID_TRANSCODER_ENABLE,$(DVD_CFLAGS)),STVID_TRANSCODER_ENABLE)
    VID_OPTIONAL_TRANSCODECS := mpg2toh264
    VID_OPTIONAL_VIDCODECS += vcodmpg2
    VID_OPTIONAL_MODULES := transcoder $(VID_OPTIONAL_MODULES)
    # Remove avsync for transcode
    VID_OPTIONAL_MODULES := $(subst avsync,,$(VID_OPTIONAL_MODULES))
  endif
  ifeq "$(STVID_USE_DMPEG_AS_CODEC)" "YES"
    VID_OPTIONAL_VIDCODECS += vcodmpg2
    VID_DEFS := $(VID_DEFS) -DSTVID_USE_DMPEG_AS_CODEC
  endif
  ifeq ($(DVD_STVIN),MASTER)
    VID_OPTIONAL_FEATURES += MasterDigInput diginput
  else
    VID_OPTIONAL_MODULES += diginput
  endif
  ifneq ($(VIDEO_MINIMIZE_MEMORY_USAGE_FOR_DEI),)
    VID_DEFS := $(VID_DEFS) -DSTVID_MINIMIZE_MEMORY_USAGE_FOR_DEI
  endif
endif

ifeq "$(DVD_BACKEND)" "ZEUS"
  VID_HW_CELLS := $(VID_HW_CELLS) lcmpegx1 sddispo2 genbuff displaypipe
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) decode inject producer ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) multicodec oldmpeg2codec
  VID_OPTIONAL_VIDCODECS := vcodh264 vcodvc1
endif

ifeq "$(DVD_BACKEND)" "7200"
  VID_HW_CELLS := $(VID_HW_CELLS) sddispo2 genbuff displaypipe
  VID_OPTIONAL_MODULES := $(VID_OPTIONAL_MODULES) inject producer ordqueue
  VID_OPTIONAL_FEATURES := $(VID_OPTIONAL_FEATURES) multicodec multifieldpresentation displaydelayline
  VID_OPTIONAL_VIDCODECS := vcodh264 vcodmpg4p2 vcodvc1 vcodmpg2
  ifeq ($(DVD_STVIN),MASTER)
    VID_OPTIONAL_FEATURES += MasterDigInput diginput
  else
    VID_OPTIONAL_MODULES += diginput
  endif
  ifdef STVID_USE_FGT
    VID_OPTIONAL_MODULES += fgt
    VID_DEFS := $(VID_DEFS) -DST_XVP_ENABLE_FLEXVP -DST_XVP_ENABLE_FGT -DSTVID_USE_FGT
  endif
endif

ifeq "$(DVD_BACKEND)" "DELTAPHI_HE"
  VID_HW_CELLS := genbuff sddispo2
  VID_OPTIONAL_MODULES   := producer ordqueue inject display
  VID_OPTIONAL_FEATURES  := multicodec
  VID_OPTIONAL_VIDCODECS := vcodh264 vcodmpg4p2
endif

ifeq "$(DVD_BACKEND)" "DELTAMU_HE"
  VID_HW_CELLS := genbuff sddispo2
  VID_OPTIONAL_MODULES   := $(VID_OPTIONAL_MODULES) producer ordqueue inject he_disp
  VID_OPTIONAL_FEATURES  := $(VID_OPTIONAL_FEATURES) multicodec
  VID_OPTIONAL_VIDCODECS := vcodh264 vcodvc1 vcodmpg2
endif

ifdef STVID_7100_SOFTWARE_PLATFORM
  VID_HW_CELLS := genbuff sddispo2
  VID_OPTIONAL_MODULES   := producer ordqueue inject display
  VID_OPTIONAL_FEATURES  := multicodec
  VID_OPTIONAL_VIDCODECS := vcodh264
endif

ifdef STVID_7109_SOFTWARE_PLATFORM
  VID_HW_CELLS := genbuff sddispo2 displaypipe
  VID_OPTIONAL_MODULES   := producer ordqueue inject display
  VID_OPTIONAL_FEATURES  := multicodec
  VID_OPTIONAL_VIDCODECS := vcodh264
endif

ifdef STVID_7200_SOFTWARE_PLATFORM
  VID_HW_CELLS := genbuff sddispo2 displaypipe
  VID_OPTIONAL_MODULES   := producer ordqueue inject display
  VID_OPTIONAL_FEATURES  := multicodec
  VID_OPTIONAL_VIDCODECS := vcodh264
endif

# rule to enable quick mpeg2 codec based on decode module for producer.
# The actual running platform is 5517 on mb361 or mb382
ifdef STVID_7100MPEG_SOFTWARE_PLATFORM
  VID_HW_CELLS := omega1
ifeq "$(DVD_BACKEND)" "7020"
    VID_HW_CELLS := omega2
endif
  VID_OPTIONAL_MODULES := producer ordqueue display avsync decode
  VID_OPTIONAL_FEATURES  := multicodec Mpeg2CodecFromOldDecodeModule avsync
  VID_OPTIONAL_VIDCODECS := vcodmpg2
endif

ifneq ($(STVID_SUPPORT_ONLY_SD_PROFILES),)
  VID_DEFS := $(VID_DEFS) -DSUPPORT_ONLY_SD_PROFILES
endif

ifdef STVID_CODEC_ONLY
  VID_OPTIONAL_MODULES = producer ordqueue inject
endif

# add ST_producer flag if decode module is compiled but not producer module
ifeq ($(findstring producer,$(VID_OPTIONAL_MODULES)),)
  ifeq ($(findstring decode,$(VID_OPTIONAL_MODULES)),decode)
    VID_DEFS := $(VID_DEFS) -D ST_producer
    INCLUDE_PATH += $(DVD_BUILD_DIR)/producer
  endif
endif

# Remove trickmod with no decode module
ifeq ($(findstring decode,$(VID_OPTIONAL_MODULES)),)
  VID_OPTIONAL_MODULES := $(subst trickmod,,$(VID_OPTIONAL_MODULES))
  VID_OPTIONAL_FEATURES := $(subst smoothbackward,,$(VID_OPTIONAL_FEATURES))
endif

# Add define for no transcoder module
ifneq ($(findstring transcoder,$(VID_OPTIONAL_MODULES)),)
  VID_DEFS := $(VID_DEFS) -D STVID_USE_ENCODER
endif

# Add define for test application to be informed of hardware error
ifneq ($(STVID_HARDWARE_ERROR_EVENT),)
  VID_DEFS := $(VID_DEFS) -D STVID_HARDWARE_ERROR_EVENT
endif

# Add define to enable debug traces in producer and codecs
ifneq ($(STVID_DEBUG_PRODUCER),)
  VID_DEFS := $(VID_DEFS) -D STVID_DEBUG_PRODUCER
endif

# Add define to enable decode time measurement from LX
ifneq ($(STVID_VALID_MEASURE_TIMING),)
  VID_DEFS := $(VID_DEFS) -DSTVID_VALID_MEASURE_TIMING
endif

# Add optional features compilation
VID_DEFS := $(VID_DEFS) $(addprefix -D ST_,$(VID_OPTIONAL_FEATURES))

#Uncomment this to perform measures
#ST20_OBJS := $(ST20_OBJS) measures.tco
#VID_DEFS := $(VID_DEFS) -D STVID_MEASURES
#IMPORTS := $(IMPORTS) stcommon

# Set task priorities according to environment variables, but don't overwrite if they are already set in DVD_CFLAGS
ifneq ($(STVID_TASK_PRIORITY_DECODE),)
  ifeq ($(findstring STVID_TASK_PRIORITY_DECODE,$(DVD_CFLAGS)),)
    VID_DEFS := $(VID_DEFS) -DSTVID_TASK_PRIORITY_DECODE=$(STVID_TASK_PRIORITY_DECODE)
  endif
endif
ifneq ($(STVID_TASK_PRIORITY_DISPLAY),)
  ifeq ($(findstring STVID_TASK_PRIORITY_DISPLAY,$(DVD_CFLAGS)),)
    VID_DEFS := $(VID_DEFS) -DSTVID_TASK_PRIORITY_DISPLAY=$(STVID_TASK_PRIORITY_DISPLAY)
  endif
endif
ifneq ($(STVID_TASK_PRIORITY_ERROR_RECOVERY),)
  ifeq ($(findstring STVID_TASK_PRIORITY_ERROR_RECOVERY,$(DVD_CFLAGS)),)
    VID_DEFS := $(VID_DEFS) -DSTVID_TASK_PRIORITY_ERROR_RECOVERY=$(STVID_TASK_PRIORITY_ERROR_RECOVERY)
  endif
endif
ifneq ($(STVID_TASK_PRIORITY_TRICKMODE),)
  ifeq ($(findstring STVID_TASK_PRIORITY_TRICKMODE,$(DVD_CFLAGS)),)
    VID_DEFS := $(VID_DEFS) -DSTVID_TASK_PRIORITY_TRICKMODE=$(STVID_TASK_PRIORITY_TRICKMODE)
  endif
endif
ifneq ($(STVID_TASK_PRIORITY_SPEED),)
  ifeq ($(findstring STVID_TASK_PRIORITY_SPEED,$(DVD_CFLAGS)),)
    VID_DEFS := $(VID_DEFS) -DSTVID_TASK_PRIORITY_SPEED=$(STVID_TASK_PRIORITY_SPEED)
  endif
endif
ifneq ($(STVID_TASK_PRIORITY_INJECTERS),)
  ifeq ($(findstring STVID_TASK_PRIORITY_INJECTERS,$(DVD_CFLAGS)),)
    VID_DEFS := $(VID_DEFS) -DSTVID_TASK_PRIORITY_INJECTERS=$(STVID_TASK_PRIORITY_INJECTERS)
  endif
endif

# Only enable assert() calls if DEBUG=1
ifeq ($(DEBUG),)
    DVD_CFLAGS += -DNDEBUG
endif


ifndef STVID_DISABLE_PERF					# set STVID_DISABLE_PERF to disable perf1.task2. (This is always disabled in linux)
	DVD_CFLAGS += -DPERF1_TASK2
endif

# Enable Security features if DVD_SECURED_CHIP is set.
ifneq ($(DVD_SECURED_CHIP),)
  VID_DEFS := $(VID_DEFS) -D DVD_SECURED_CHIP
endif

ifneq ($(findstring lcmpegx1,$(VID_HW_CELLS)),)
  VID_DEFS := $(VID_DEFS) -D LCMPEGX1_WA_GNBvd45748
endif

ifneq ($(STVID_WA_GNBvd62704),)
	VID_DEFS := $(VID_DEFS) -D WA_GNBvd62704
endif


# Pass definitions to sub make calls
export VID_DEFS
export VID_HW_CELLS := $(sort $(VID_HW_CELLS))
export VID_OPTIONAL_MODULES
export VID_OPTIONAL_FEATURES
export VID_OPTIONAL_VIDCODECS
export VID_OPTIONAL_TRANSCODECS

VID_MODULES := $(VID_MODULES) $(VID_OPTIONAL_MODULES)

SUBDIRS := $(VID_MODULES)

# Compile options
CFLAGS := $(VID_DEFS) $(CFLAGS)

# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/..

include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# Local dependancies
vid_ctcm.tco vid_ctcm.o: vid_ctcm.h vid_com.h

#end of file
