############################
# MODULE : decoder.lib  #
############################

VID_MODULE_NAME = decoder

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := avsdecode.tco

ifdef STUB_AVS_FMW_ST40
ST20_OBJS += avsstub.tco
endif

IMPORTS := sttbx stevt stos

ifdef STVID_TRACE_BUFFER_UART
   INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../../tests/src
endif

ST40_IMPORTS := $(IMPORTS)
ST20_TARGETS := $(VID_MODULE_NAME).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

ifneq ($(STVID_DECODER_TRACE_UART),)
# Define TRACE_DECODER to compile vc1decoder with UART traces
	CFLAGS := $(CFLAGS) -DTRACE_DECODER
# Note : Set also STVID_TRACE_BUFFER_UART in order to link test application with trace.tco
endif

ifdef MME_DUMP
	CFLAGS := $(CFLAGS) -DMME_DUMP
endif

ifdef DUMP_ALL_FIELDS
	CFLAGS := $(CFLAGS) -DUMP_ALL_FIELDS
endif

ifdef MME_DUMP_WITH_ADDR
	CFLAGS := $(CFLAGS) -DMME_DUMP_WITH_ADDR
endif

ifdef MME_DUMP_WITH_SCALING_LISTS
	CFLAGS := $(CFLAGS) -DMME_DUMP_WITH_SCALING_LISTS
endif

ifdef STUB_MPEG4P2_FMW_ST40
	CFLAGS := $(CFLAGS) -DSTUB_AVS_FMW_ST40
endif

ifdef STUB_FMW_LX
	CFLAGS := $(CFLAGS) -DSTUB_FMW_LX
endif

ifdef STUB_PP
	CFLAGS := $(CFLAGS) -DSTUB_PP
endif

ifdef STVID_VALID
	CFLAGS := $(CFLAGS) -DST_producer
endif


# General rules
CFLAGS := -D__OS21__ -D__SH4__ -DOSAL_OS21 -DHAL_ST40 -DMME20 -DMULTICOM $(VID_DEFS) $(CFLAGS)
ifeq ($(STVID_NO_TRICKMODE),)
    ifneq ($(findstring $(DVD_BACKEND),"7109 7100 7200"),)
	CFLAGS := $(CFLAGS) -DST_speed
    endif
endif

# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../../.. $(DVD_BUILD_DIR)/../../../..
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../inject
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../api
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../buffers
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../ordqueue
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../display
INCLUDE_PATH += $(RPC_ROOT)/include
INCLUDE_PATH += $(RPC_ROOT)/src/mme/include
INCLUDE_PATH += $(RPC_ROOT)/src/embx/include

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
#vc1dumpmme.tco vc1dumpmme.o : mpg4p2decode.h
avsstub.tco avsstub.o : avsdecode.h
avsdecode.tco avsdecode.o : avsdecode.h
#vc1task.tco vc1task.o : vc1decode.h
#end of file
