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
ifeq "$(DVD_BACKEND)" "5197"
    OUTPUT = 5197
    HEADER_IMPORTS +=stfdma
endif

ifdef ST_51XX
    OUTPUT = 51XX
     HEADER_IMPORTS +=stfdma
endif

EXPORTS := softaudio_codecinterface.lib
TARGETS := softaudio_codecinterface.lib

# local objects which comprise this component

OBJS = softaudio_codecinterface.tco

ifndef ST_5197
OBJS += mixer_multicom.tco st20_util.tco mixer_process.tco mixer_tools.tco
endif



COMMON_SUBDIRS  = ..$(SLASH)common
DECODER_SUBDIRS = ..$(SLASH)decoder
PARSER_SUBDIRS  = ..$(SLASH)parser
OUTPUT_SUBDIRS  = ..$(SLASH)output$(SLASH)$(OUTPUT)
MULTICOM_SUBDIRS = ..$(SLASH)audiocodec_interface

INCLUDE_PATH += $(RPC_ROOT)$(SLASH)include
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(MULTICOM_SUBDIRS))
INCLUDE_PATH += $(DVD_BUILD_DIR)$(SLASH)..
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(COMMON_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(DECODER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(PARSER_SUBDIRS))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)$(SLASH),$(OUTPUT_SUBDIRS))
# INCLUDE_PATH +=H:\dvdno-prj-staudlx\src\audiocodec_interface



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
	STAUD_DEFS += -DDVR -ftime
endif




ifndef ST_5197
CFLAGS  := $(CFLAGS) $(STAUD_CFLAGS) $(STAUD_DEFS)
else
CFLAGS  := $(CFLAGS) $(STAUD_CFLAGS) $(STAUD_DEFS)
endif

# export STAUD_CFLAGS to other STAUD makefiles and add
# to CFLAGS for this makefile


ifdef ST_5197
COMMENT = ********** WARNING ************* For the time being we built codec software.... Remove this compilation, once codec libs are available*************

CODEC_LIBS = libsoftcodec_V0.1.a

# Remove above lib name if want to build codec by own

ifeq "$(CODEC_LIBS)" ""
	SUBDIRS := softcodec
else
	COMMENT = ******* Used Codec lib is $(CODEC_LIBS) ********
	SUBDIRS :=
endif

else

SUBDIRS :=

# Write the name of Codec lib
# used for Mixer Purpuse .. Still No name for this

CODEC_LIBS =



endif

LINK_PATH += ..$(SLASH)..$(SLASH)codeclib


include $(DVD_MAKE)/defrules.mak

$(CODEC_LIBS) :


# Build lib and obj
#------------------

$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS) $(CODEC_LIBS)
	@$(ECHO) Building  $@
	$(BUILD_LIBRARY)
	@$(ECHO) $(COMMENT)

ifeq "$(ARCHITECTURE)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS) $(CODEC_LIBS)
	@$(ECHO) Building   $@
	$(BUILD_LIBRARY) $@
	@$(ECHO) $(COMMENT)
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
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)


FORCE:

else

include $(DVD_MAKE)/builddir.mak

endif
# End of STAUD ./src level makefile
