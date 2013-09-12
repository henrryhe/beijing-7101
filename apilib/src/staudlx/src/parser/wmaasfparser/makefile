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

EXPORTS := wmaasfparser.lib 
TARGETS := wmaasfparser.lib

# local objects which comprise this component
OBJS =  es_wma_parser.tco


COMMON_SUBDIRS  = ..$(SLASH)..$(SLASH)common
DECODER_SUBDIRS = ..$(SLASH)..$(SLASH)decoder
OUTPUT_SUBDIRS  = ..$(SLASH)..$(SLASH)output
MULTICOM_SUBDIRS = ..$(SLASH)..$(SLASH)audiocodec_interface
PARSER_SRC			= ..$(SLASH)..$(SLASH)parser
AUDIO_SRC			= ..$(SLASH)..$(SLASH)..$(SLASH)src

INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(COMMON_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(DECODER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(OUTPUT_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(OUTPUT_SUBDIRS)/$(outputsubdir))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(MULTICOM_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(PARSER_SRC))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(AUDIO_SRC))
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



ifeq "$(DVR)" "1"
CFLAGS := $(CFLAGS) -DDVR
endif


ifdef STAUD_REMOVE_CLKRV_SUPPORT
STAUD_CFLAGS := $(STAUD_CFLAGS) -D STAUD_REMOVE_CLKRV_SUPPORT
endif
ifdef STAUD_REMOVE_PTI_SUPPORT
STAUD_CFLAGS := $(STAUD_CFLAGS) -D STAUD_REMOVE_PTI_SUPPORT
endif

ifdef STAUD_DEBUG
    STAUD_CFLAGS := $(STAUD_CFLAGS) -D STAUD_DEBUG -D STTBX_REPORT -D STTBX_PRINT
endif

ifeq "$(DVD_TOOLSET)" "ST40WINCE"
	CFLAGS += $(STAUD_CFLAGS) /FI"$(AUDIO_INCLUDE)"
else
	ifndef ST_51XX

	else
		CFLAGS += $(STAUD_CFLAGS) -I $(AUDIO_INCLUDE)
	endif
endif	

CFLAGS  := $(CFLAGS) $(STAUD_CFLAGS) $(STAUD_DEFS)
# export STAUD_CFLAGS to other STAUD makefiles and add
# to CFLAGS for this makefile

 


include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
ifeq "$(ARCHITECTURE)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif
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
