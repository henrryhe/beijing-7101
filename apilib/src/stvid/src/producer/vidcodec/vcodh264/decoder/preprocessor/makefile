##############################
# MODULE : preprocessor.lib  #
##############################

VID_MODULE_NAME = preprocessor

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := h264preproc_transformer.tco h264preproc.tco
ifdef STVID_PP_DUMP
    ST20_OBJS += h264preproc_dump.tco
else
ifdef STVID_BITBUFFER_DUMP
    ST20_OBJS += h264preproc_dump.tco
endif
endif

ST40_IMPORTS := $(IMPORTS) stos stsys
ST20_TARGETS := $(VID_MODULE_NAME).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

ifneq ($(STVID_DECODER_TRACE_UART),)
# Define TRACE_DECODER to compile h264decoder with UART traces
	CFLAGS := $(CFLAGS) -DTRACE_DECODER
# Note : Set also STVID_TRACE_BUFFER_UART in order to link test application with trace.tco
endif

ifdef STVID_PP_DUMP
	CFLAGS := $(CFLAGS) -DSTVID_PP_DUMP
endif

ifdef STVID_BITBUFFER_DUMP
	CFLAGS := $(CFLAGS) -DSTVID_BITBUFFER_DUMP
endif

ifdef STVID_USE_ONE_PREPROC
	CFLAGS := $(CFLAGS) -DSTVID_USE_ONE_PREPROC
endif

ifdef STVID_VALID
	CFLAGS := $(CFLAGS) -DST_producer
endif

ifdef STVID_DEBUG_PREPROC
	CFLAGS := $(CFLAGS) -DSTVID_DEBUG_PREPROC
endif

# General rules
ifeq "$(DVD_TOOLSET)" "ST40WINCE"
CFLAGS := -D__WINCE__ -D__SH4__ -DOSAL_OS21 -DHAL_ST40 -DMME20 $(VID_DEFS) $(CFLAGS)
else
CFLAGS := -D__OS21__ $(VID_DEFS) $(CFLAGS)
endif

# Add include paths
# add decoder dir in include path
INCLUDE_PATH += $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/../../.. $(DVD_BUILD_DIR)/../../../../..
# add multicom dirs in include path
INCLUDE_PATH += $(RPC_ROOT)/include

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

clean_message: FORCE
	@$(ECHO) Cleaning $(VID_MODULE_NAME) module

clean: clean_message $(SUBDIR_CLEAN)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

FORCE:

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependencies
#end of file
