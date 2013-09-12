# STAUD ./src level makefile

# By default support all possible hardware cells

#CFLAGS  := -O3 -fomit-frame-pointer -funroll-loops -funroll-all-loops  $(CFALGS)

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS := stpti4 stsys stavmem stcommon   sttbx stos stfdma

# For the ST 5197 we are building code usingf DVD_BACKEND 7100, So There is below condition , Once support for DVD_BACKEND 5197 support will be avaiable
# remove the condition below

ifdef ST_5197
	outputsubdir = 7100
	STAUD_CFLAGS := $(STAUD_CFLAGS) -DST_5197
else
	ifeq "$(DVD_BACKEND)" "7100"
	    outputsubdir = 7100
	endif
	ifeq "$(DVD_BACKEND)" "7109"
	    outputsubdir = 7100
	endif
	ifeq "$(DVD_BACKEND)" "7200"
	    outputsubdir = 7200
	endif
endif

ifdef ST_51XX
	outputsubdir = 51XX
endif

EXPORTS := staudlx.lib staudlx.h
TARGETS := staudlx.lib

# local objects which comprise this component
ifdef ST_51XX
OBJS = staud_amphion.tco \
       aud_wrap_amphion.tco
else
OBJS = staud.tco \
       aud_wrap.tco
endif

ifeq "$(STAUDLX_UNINSTALL_AC3_DDP)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_AC3_DDP
endif

ifeq "$(STAUDLX_UNINSTALL_MPEG)" "1"
	STAUD_CFLAGS += -DSTAUDLX_UNINSTALL_MPEG
endif

ifeq "$(STAUDLX_UNINSTALL_HEAAC)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_HEAAC
endif

ifeq "$(STAUDLX_UNINSTALL_DTS)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_DTS
endif

ifeq "$(STAUDLX_UNINSTALL_CDDA_PCM)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_CDDA_PCM
endif

ifeq "$(STAUDLX_UNINSTALL_LPCMV)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_LPCMV
endif

ifeq "$(STAUDLX_UNINSTALL_LPCMA)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_LPCMA
endif

ifeq "$(STAUDLX_UNINSTALL_WMA)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_WMA
endif

ifeq "$(STAUDLX_UNINSTALL_WMAPRO)" "1"
	STAUD_CFLAGS +=  -DSTAUDLX_UNINSTALL_WMAPRO
endif

ifdef ENABLE_BTSC
STAUD_CFLAGS +=  -DENABLE_BTSC
endif

ifdef ENABLE_TRANSCODED_DTS_IN_PCM_MODE
	STAUD_CFLAGS +=  -DENABLE_TRANSCODED_DTS_IN_PCM_MODE
endif

ifdef ST_51XX

	AUD_MODULES =  common parser output decoder
	ifdef AD_SUPPORT
		AUD_MODULES +=  mixer softaudio_codecinterface
		STAUD_CFLAGS +=  -DMIXER_SUPPORT
	endif
	ifdef PCM_SUPPORT
		STAUD_CFLAGS +=  -DPCM_SUPPORT
	endif
	ifdef INPUT_PCM_SUPPORT
		AUD_MODULES +=  input
		STAUD_CFLAGS +=  -DINPUT_PCM_SUPPORT
	endif
	ifdef FRAME_PROCESSOR_SUPPORT
		STAUD_CFLAGS +=  -DFRAME_PROCESSOR_SUPPORT
	endif

else
	AUD_MODULES = common decoder output parser mixer input encoder post_processor
endif
ifdef ST_5197
	AUD_MODULES += softaudio_codecinterface
endif

INCLUDE_PATH += $(RPC_ROOT)$(SLASH)include
INCLUDE_PATH += $(DVD_BUILD_DIR)$(SLASH)output$(SLASH)$(outputsubdir)
INCLUDE_PATH += $(DVD_BUILD_DIR)$(SLASH)audiocodec_interface

ST20_EXPORTS := $(EXPORTS)
ST20_TARGETS := $(TARGETS)
ST20_OBJS := $(OBJS)
ST40_IMPORTS := $(IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(OBJS))

ST200_IMPORTS := $(IMPORTS)
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(TARGETS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(OBJS))


export STAUD_CFLAGS

CFLAGS += $(STAUD_CFLAGS)

ifdef STAUD_REMOVE_CLKRV_SUPPORT
	CFLAGS += -D STAUD_REMOVE_CLKRV_SUPPORT
endif

ifdef STAUD_REMOVE_PTI_SUPPORT
	CFLAGS += -D STAUD_REMOVE_PTI_SUPPORT
endif

ifdef STAUD_VERBOSE_PRINT
	CFLAGS += -D STAUD_VERBOSE_PRINT
endif

ifeq "$(DVR)" "1"
	CFLAGS := $(CFLAGS) -DDVR
endif

ifeq "$(MSPP_PARSER)" "1"
	CFLAGS +=  -D MSPP_PARSER
endif



SUBDIRS := $(AUD_MODULES)

include $(DVD_MAKE)/defrules.mak


ifeq "$(ARCHITECTURE)" "ST20"
$(ST20_TARGETS): $(SUBDIR_LIBS) $(ST20_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif

ifeq "$(ARCHITECTURE)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif





ifeq "$(ARCHITECTURE)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif



clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)





FORCE:

else

include $(DVD_MAKE)/builddir.mak

endif






