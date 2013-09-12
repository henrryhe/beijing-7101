# STAUD ./src level makefile

# By default support all possible hardware cells


DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS := stpti4 stsys  stavmem stcommon   sttbx stos pcm_spdif stfdma

# For the ST 5197 we are building code using DVD_BACKEND 7100, So There is below condition , Once support for DVD_BACKEND 5197 support will be avaiable
# remove the following condition

CFLAGS := $(CFLAGS) -DPCMP0_SUPPORTED -DSPDIFP0_SUPPORTED

ifdef ST_5197
	OUTPUT = 7100
#	CFLAGS := $(CFLAGS) -DBYTES_PER_CHANNEL_IN_PLAYER=4 -DTENCH_TRANSFER_COUNT=10 -DPCMP1_SUPPORTED -DTW0CH_TRANSFER_COUNT=4
endif 
	ifeq "$(DVD_BACKEND)" "7100"
	    OUTPUT = 7100
		CFLAGS := $(CFLAGS) -DBYTES_PER_CHANNEL_IN_PLAYER=4 -DTENCH_TRANSFER_COUNT=10 -DPCMP1_SUPPORTED -DTW0CH_TRANSFER_COUNT=4
	endif
	ifeq "$(DVD_BACKEND)" "7109"
	    OUTPUT = 7100
		CFLAGS := $(CFLAGS) -DBYTES_PER_CHANNEL_IN_PLAYER=4 -DTENCH_TRANSFER_COUNT=20 -DPCMP1_SUPPORTED -DTW0CH_TRANSFER_COUNT=4
	endif
	ifeq "$(DVD_BACKEND)" "7200"
	    OUTPUT = 7200
		CFLAGS := $(CFLAGS) -DBYTES_PER_CHANNEL_IN_PLAYER=4 -DTW0CH_TRANSFER_COUNT=4 -DTENCH_TRANSFER_COUNT=20 -DPCMP1_SUPPORTED -DPCMP2_SUPPORTED -DPCMP3_SUPPORTED -DHDMIPCMP0_SUPPORTED -DHDMISPDIFP0_SUPPORTED
	endif


ifdef ST_51XX
    OUTPUT = 51xx
	CFLAGS := $(CFLAGS) -DBYTES_PER_CHANNEL_IN_PLAYER=2 -DTW0CH_TRANSFER_COUNT=5 -DTENCH_TRANSFER_COUNT=5
endif



EXPORTS := output.lib
TARGETS := output.lib



# local objects which comprise this component

OBJS = aud_pcmplayer.tco \
       aud_spdifplayer.tco \
       aud_pcmplayer_$(OUTPUT).tco \
       aud_spdifplayer_$(OUTPUT).tco

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

# Set task priorities according to environment variables, but don't overwrite if they are already set in DVD_CFLAGS
#DVD_CFLAGS := $(DVD_CFLAGS) -DPCMPLAYER_TASK_PRIORITY=2 -DSPDIFPLAYER_TASK_PRIORITY=2 -DAUDIO_PP_TASK_PRIORITY=2


ifneq ($(strip $(PCMPLAYER_TASK_PRIORITY)),)
  ifeq ($(findstring PCMPLAYER_TASK_PRIORITY,$(DVD_CFLAGS)),)
    STAUD_DEFS += -DPCMPLAYER_TASK_PRIORITY=$(PCMPLAYER_TASK_PRIORITY)
  endif
endif

ifneq ($(strip $(SPDIFPLAYER_TASK_PRIORITY)),)
  ifeq ($(findstring SPDIFPLAYER_TASK_PRIORITY,$(DVD_CFLAGS)),)
    STAUD_DEFS += -DSPDIFPLAYER_TASK_PRIORITY=$(SPDIFPLAYER_TASK_PRIORITY)
  endif
endif

ifneq ($(strip $(AUDIO_PP_TASK_PRIORITY)),)
  ifeq ($(findstring AUDIO_PP_TASK_PRIORITY,$(DVD_CFLAGS)),)
    STAUD_DEFS += -DAUDIO_PP_TASK_PRIORITY=$(AUDIO_PP_TASK_PRIORITY)
  endif
endif

ifeq "$(MSPP_PARSER)" "1"
	STAUD_DEFS += -D MSPP_PARSER
endif


ifdef ENABLE_SPDIF_INTERRUPT
STAUD_DEFS += -D ENABLE_SPDIF_INTERRUPT
endif


#export STAUD_CFLAGS
CFLAGS  := $(CFLAGS) $(STAUD_CFLAGS) $(STAUD_DEFS)
# export STAUD_CFLAGS to other STAUD makefiles and add
# to CFLAGS for this makefile

include $(DVD_MAKE)/defrules.mak

# Pick up standars rules
#-----------------------

# Additional rules for files not in current directory
 aud_pcmplayer$(OBJ_SUFFIX) : aud_pcmplayer.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
 aud_spdifplayer$(OBJ_SUFFIX): aud_spdifplayer.c
	@$(ECHO) Compiling $<
	$(COMPILE_C)

 aud_pcmplayer_$(OUTPUT)$(OBJ_SUFFIX) : $(OUTPUT)/aud_pcmplayer_$(OUTPUT).c
	@$(ECHO) Compiling $<
	$(COMPILE_C)
 aud_spdifplayer_$(OUTPUT)$(OBJ_SUFFIX): $(OUTPUT)/aud_spdifplayer_$(OUTPUT).c
	@$(ECHO) Compiling $<
	$(COMPILE_C)


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
