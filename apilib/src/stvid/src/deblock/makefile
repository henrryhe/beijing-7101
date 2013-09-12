############################
# MODULE : deblock.lib     #
############################

VID_MODULE_NAME = deblock

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := deblock.tco

IMPORTS := sttbx stavmem stos

ifdef VALID_TOOLS
     IMPORTS += valitls
else
     ifdef STVID_TRACE_BUFFER_UART
         INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../../tests/src
     endif
endif

ST40_IMPORTS := $(IMPORTS)
ST20_TARGETS := $(VID_MODULE_NAME).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

ifneq ($(STVID_DECODER_TRACE_UART),)
# Define TRACE_DECODER to compile with UART traces
	CFLAGS := $(CFLAGS) -D TRACE_DECODER
# Note : Set also STVID_TRACE_BUFFER_UART in order to link test application with trace.tco
endif

ifdef NO_DECODE
	CFLAGS := $(CFLAGS) -DNO_DECODE
endif

ifdef MME_DUMP
	CFLAGS := $(CFLAGS) -DMME_DUMP
endif

ifdef STVID_DEBUG_DECODER
	CFLAGS := $(CFLAGS) -DSTVID_DEBUG_DECODER
endif

# General rules
CFLAGS := -D__OS21__ $(VID_DEFS) $(CFLAGS)

# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../../.. $(DVD_BUILD_DIR)/../../../..
INCLUDE_PATH += $(RPC_ROOT)/include
INCLUDE_PATH += $(RPC_ROOT)/src/mme/include
INCLUDE_PATH += $(RPC_ROOT)/src/embx/include
INCLUDE_PATH += $(DVD_BUILD_DIR)/../decode
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/,stapigat/src/valid))
else
    INCLUDE_PATH += $(wildcard $(addprefix $(DVD_ROOT)/*-prj-,stapigat/src/valid))
endif

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
deblock.tco deblock.o : deblock.h mpeg2_videodeblocktransformertypes.h 
#end of file
