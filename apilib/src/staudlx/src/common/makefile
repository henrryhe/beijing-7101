# STAUD ./src level makefile

# By default support all possible hardware cells


DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS := stpti4 stsys stavmem stcommon   sttbx stos stfdma
ifeq "$(DVD_BACKEND)" "7100"
    outputsubdir = 7100
endif
ifeq "$(DVD_BACKEND)" "7109"
    outputsubdir = 7100
endif
ifeq "$(DVD_BACKEND)" "7200"
    outputsubdir = 7200
endif
ifdef ST_51XX
    outputsubdir = 51xx
endif

EXPORTS := common.lib BlockManager.h aud_utils.h acc_multicom_toolbox.h
TARGETS := common.lib

# local objects which comprise this component
ifdef ST_51XX
	OBJS = aud_utils.tco \
		   aud_evt.tco \
	       blockmanager.tco \
	       acc_multicom_toolbox.tco \
	       staud_core_amp.tco
else
	OBJS = acc_multicom_toolbox.tco \
		acc_mme_wrapper.tco \
		   aud_utils.tco \
		   aud_evt.tco \
	       blockmanager.tco \
	       staud_core.tco 
endif

ifdef STAUD_VERBOSE_PRINT 
	OBJS += staud_prints.tco 
endif 


PARSER_SUBDIRS  = ..$(SLASH)parser
DECODER_SUBDIRS = ..$(SLASH)decoder
OUTPUT_SUBDIRS  = ..$(SLASH)output
INPUT_SUBDIRS  = ..$(SLASH)input
ENCODER_SUBDIRS = ..$(SLASH)encoder
MIXER_SUBDIRS = ..$(SLASH)mixer
PP_SUBDIRS = ..$(SLASH)post_processor
COMMON_SUBDIRS  = ..$(SLASH)common
MULTICOM_SUBDIRS = ..$(SLASH)audiocodec_interface
SOFTCODEC_SUBDIRS = ..$(SLASH)softaudio_codecinterface

INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(PARSER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(INPUT_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(ENCODER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(DECODER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(MIXER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(PP_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(OUTPUT_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(OUTPUT_SUBDIRS)$(SLASH)$(outputsubdir))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(MULTICOM_SUBDIRS))
INCLUDE_PATH += $(DVD_BUILD_DIR)$(SLASH)..
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(SOFTCODEC_SUBDIRS))

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
CFLAGS := $(CFLAGS) -DDVR
endif

ifeq "$(MSPP_PARSER)" "1"
	CFLAGS += -D MSPP_PARSER
endif

ifdef STAUD_VERBOSE_PRINT
	CFLAGS += -D STAUD_VERBOSE_PRINT	
endif 

# Set task priorities according to environment variables, but don't overwrite if they are already set in DVD_CFLAGS
#DVD_CFLAGS := $(DVD_CFLAGS) -DDATAPROCESSER_TASK_PRIORITY=2

ifneq ($(strip $(DATAPROCESSER_TASK_PRIORITY)),)
  ifeq ($(findstring DATAPROCESSER_TASK_PRIORITY,$(DVD_CFLAGS)),)
    STAUD_DEFS += -DDATAPROCESSER_TASK_PRIORITY=$(DATAPROCESSER_TASK_PRIORITY)
  endif
endif

#export STAUD_CFLAGS
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
