# STAUD ./src level makefile

# By default support all possible hardware cells


DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS := stpti4 stsys  stavmem stcommon sttbx stos pcm_spdif
ifeq "$(DVD_BACKEND)" "7100"
    OUTPUT = 7100
    HEADER_IMPORTS +=stfdma
endif
ifeq "$(DVD_BACKEND)" "7109"
    OUTPUT = 7100
    HEADER_IMPORTS +=stfdma
endif
ifeq "$(DVD_BACKEND)" "8010"
    OUTPUT = 8010
    HEADER_IMPORTS +=stflexidma
endif
ifeq "$(DVD_BACKEND)" "5525"
    OUTPUT = 5525
    HEADER_IMPORTS +=stfdma
endif
ifeq "$(DVD_BACKEND)" "7200"
    OUTPUT = 7200
    HEADER_IMPORTS +=stfdma
endif
EXPORTS := post_processor.lib
TARGETS := post_processor.lib

# local objects which comprise this component

OBJS = aud_mmepp.tco

COMMON_SUBDIRS  = ..$(SLASH)common
DECODER_SUBDIRS = ..$(SLASH)decoder
PARSER_SUBDIRS  = ..$(SLASH)parser
OUTPUT_SUBDIRS  = ..$(SLASH)output$(SLASH)$(OUTPUT)
MULTICOM_SUBDIRS = ..$(SLASH)audiocodec_interface

INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(COMMON_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(DECODER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(PARSER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(OUTPUT_SUBDIRS))
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

ifdef STAUD_USE_PCM_PROCESS_TRANSFORMER
	STAUD_DEFS += -DSTAUD_PCM_PROCESSOR
endif

# Set task priorities according to environment variables, but don't overwrite if they are already set in DVD_CFLAGS
#DVD_CFLAGS := $(DVD_CFLAGS) -DAUDIO_PP_TASK_PRIORITY=2

ifneq ($(strip $(AUDIO_PP_TASK_PRIORITY)),)
  ifeq ($(findstring AUDIO_PP_TASK_PRIORITY,$(DVD_CFLAGS)),)
    STAUD_DEFS += -DAUDIO_PP_TASK_PRIORITY=$(AUDIO_PP_TASK_PRIORITY)
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
