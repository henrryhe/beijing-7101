# STAUD ./src level makefile

# By default support all possible hardware cells


DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS := stpti4 stsys stavmem stcommon   sttbx stfdma stos

EXPORTS := encoder.lib
TARGETS := encoder.lib

# local objects which comprise this component
OBJS = aud_mmeaudiostreamencoder.tco

COMMON_SUBDIRS  = ..$(SLASH)common
PARSER_SUBDIRS  = ..$(SLASH)parser
OUTPUT_SUBDIRS  = ..$(SLASH)output
DECODER_SUBDIRS = ..$(SLASH)decoder
MULTICOM_SUBDIRS = ..$(SLASH)audiocodec_interface

INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(COMMON_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(DECODER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(PARSER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(MULTICOM_SUBDIRS))
INCLUDE_PATH += $(DVD_BUILD_DIR)$(SLASH)..

INCLUDE_PATH += $(RPC_ROOT)$(SLASH)include
ST20_IMPORTS := $(IMPORTS)
ST20_TARGETS := $(TARGETS)
ST20_OBJS := $(OBJS)
ST40_IMPORTS := $(IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(TARGETS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(OBJS))
ST200_IMPORTS := $(IMPORTS)
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(TARGETS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(OBJS))

STAUD_DEFS :=

ifeq "$(DVR)" "1"
STAUD_DEFS += -DDVR
endif

# Flag to Specify whether to run WMA/WMAPro in loop or not
ifeq "$(WMA_IN_LOOP)" "1"
	STAUD_DEFS += -DWMA_IN_LOOP
endif

# Set task priorities according to environment variables, but don't overwrite if they are already set in DVD_CFLAGS
#DVD_CFLAGS := $(DVD_CFLAGS) -DSTREAMING_AUDIO_ENCODER_TASK_PRIORITY=2

ifneq ($(strip $(AUDIO_DECODER_TASK_PRIORITY)),)
  ifeq ($(findstring STREAMING_AUDIO_ENCODER_TASK_PRIORITY,$(DVD_CFLAGS)),)
    STAUD_DEFS += -DSTREAMING_AUDIO_ENCODER_TASK_PRIORITY=$(STREAMING_AUDIO_ENCODER_TASK_PRIORITY)
  endif
endif


CFLAGS  := $(CFLAGS) $(STAUD_CFLAGS) $(STAUD_DEFS)
# export STAUD_CFLAGS to other STAUD makefiles and add
# to CFLAGS for this makefile

include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
$(ST20_TARGETS): $(ST20_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
ifeq "$(ARCHITECTURE)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif
ifeq "$(ARCHITECTURE)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif

aud_mmeaudiostreamencoder$(OBJ_SUFFIX) : aud_mmeaudiostreamencoder.h

# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

FORCE:

else

include $(DVD_MAKE)/builddir.mak

endif

# End of STAUD ./src level makefile
