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

EXPORTS := parser.lib aud_pes_es_parser.h
TARGETS := parser.lib

# local objects which comprise this component
ifeq "$(MSPP_PARSER)" "1"
OBJS = aud_pes_es_parser.tco \
        mspp_parser.tco
else
	ifndef ST_51XX
	OBJS = aud_pes_es_parser.tco \
	       pes_mp2_parser.tco \
	       pes_dvdv_parser.tco \
	       pes_dvda_parser.tco \
	       pes_packmp1_parser.tco \
	       es_ac3_parser.tco \
	       es_mpeg_parser.tco \
	       es_dts_parser.tco \
	       es_lpcmv_parser.tco \
	       es_lpcma_parser.tco \
	       es_cdda_parser.tco \
	       es_mlp_parser.tco \
	       com_parser.tco \
	       parse_heaac.tco \
		   es_wmapes_parser.tco \
		   es_wave_parser.tco \
		   es_aac_parser.tco \
		   es_aiff_parser.tco
	else
		ifdef AD_SUPPORT
			OBJS = aud_pes_es_parser.tco \
			       pes_mp2_parser.tco \
		 	       es_ac3_parser_amphion.tco \
			       es_mpeg_parser.tco \
			       es_cdda_parser.tco \
			       com_parser.tco
		else
			ifdef PCM_SUPPORT
				OBJS = aud_pes_es_parser.tco \
				       pes_mp2_parser.tco \
			 	       es_ac3_parser_amphion.tco \
				       es_mpeg_parser.tco \
				       es_cdda_parser.tco \
				       com_parser.tco
			else
				OBJS = aud_pes_es_parser.tco \
				       pes_mp2_parser.tco \
			 	       es_ac3_parser_amphion.tco \
				       es_mpeg_parser.tco \
				       com_parser.tco
			endif
		endif
	endif
endif

COMMON_SUBDIRS  = ..$(SLASH)common
DECODER_SUBDIRS = ..$(SLASH)decoder
OUTPUT_SUBDIRS  = ..$(SLASH)output
MULTICOM_SUBDIRS = ..$(SLASH)audiocodec_interface
SOFTCODEC_SUBDIRS = ..$(SLASH)softaudio_codecinterface

INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(COMMON_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(DECODER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(OUTPUT_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(OUTPUT_SUBDIRS)/$(outputsubdir))
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
STAUD_DEFS += -DDVR
endif

# Set task priorities according to environment variables, but don't overwrite if they are already set in DVD_CFLAGS
#DVD_CFLAGS := $(DVD_CFLAGS) -DPES_ES_PARSER_TASK_PRIORITY=10

ifneq ($(strip $(PES_ES_PARSER_TASK_PRIORITY)),)
  ifeq ($(findstring PES_ES_PARSER_TASK_PRIORITY,$(DVD_CFLAGS)),)
    STAUD_DEFS += -DPES_ES_PARSER_TASK_PRIORITY=$(PES_ES_PARSER_TASK_PRIORITY)
  endif
endif

ifdef PES_TO_ES_BY_FDMA
	STAUD_CFLAGS +=  -DPES_TO_ES_BY_FDMA
endif

ifeq "$(MSPP_PARSER)" "1"
	STAUD_DEFS += -D MSPP_PARSER
endif

CFLAGS  := $(CFLAGS) $(STAUD_CFLAGS) $(STAUD_DEFS)

# export STAUD_CFLAGS to other STAUD makefiles and add
# to CFLAGS for this makefile

# To Build WMA ASF CODE
# Get the code from project/confidentail area
# Set the env variable WMA_PARSER_SOURCE_BUILD to 1
# To use the wma asf parser lib
# Get the lib from project/confidentail
# and put this lib in parser folder

ifdef STAUDLX_UNINSTALL_WMAPRO
	SUBDIRS :=
	WMAPARSER_LIBS =
else
	ifdef STAUDLX_UNINSTALL_WMA
		SUBDIRS :=
		WMAPARSER_LIBS =
	else
		ifdef WMA_PARSER_SOURCE_BUILD
			SUBDIRS :=wmaasfparser
			WMAPARSER_LIBS =
		else
			SUBDIRS :=
ifeq "$(DVD_TOOLSET)" "ST40WINCE"
			WMAPARSER_LIBS = libwmaasfparser_wince.a
else
			WMAPARSER_LIBS = libwmaasfparser.a
endif



		endif
	endif
endif



LINK_PATH += ..$(SLASH)..$(SLASH)..$(SLASH)parser

include $(DVD_MAKE)/defrules.mak

$(WMAPARSER_LIBS) :

# Build lib and obj
#------------------
ifeq "$(ARCHITECTURE)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif
ifeq "$(ARCHITECTURE)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)  $(WMAPARSER_LIBS)
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
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

FORCE:

else

include $(DVD_MAKE)/builddir.mak

endif

# End of STAUD ./src level makefile
