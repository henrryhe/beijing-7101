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
ST20_OBJS := h264decode.tco h264task.tco

ifdef STVID_VALID
ST20_OBJS += h264decodecmd.tco h264dumpmme.tco
endif

IMPORTS := sttbx stevt stos

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
# Define TRACE_DECODER to compile h264decoder with UART traces
	CFLAGS := $(CFLAGS) -D TRACE_DECODER
# Note : Set also STVID_TRACE_BUFFER_UART in order to link test application with trace.tco
endif

	SUBDIRS := $(SUBDIRS) preprocessor

ifdef MME_DUMP
	CFLAGS := $(CFLAGS) -DMME_DUMP
endif

ifdef MME_DUMP_WITH_ADDR
	CFLAGS := $(CFLAGS) -DMME_DUMP_WITH_ADDR
endif

ifdef MME_DUMP_WITH_SCALING_LISTS
	CFLAGS := $(CFLAGS) -DMME_DUMP_WITH_SCALING_LISTS
endif

ifdef STUB_FMW_ST40
	CFLAGS := $(CFLAGS) -DSTUB_FMW_ST40
endif

ifdef STUB_FMW_LX
	CFLAGS := $(CFLAGS) -DSTUB_FMW_LX
endif

ifdef STVID_INT_BUFFER_DUMP
	CFLAGS := $(CFLAGS) -DSTVID_INT_BUFFER_DUMP
endif

ifdef STVID_NON_STD_INT_BUFFER_NUMBER
	CFLAGS := $(CFLAGS) -DSTVID_NON_STD_INT_BUFFER_NUMBER=$(STVID_NON_STD_INT_BUFFER_NUMBER)
endif

ifdef STVID_DEBUG_DECODER
	CFLAGS := $(CFLAGS) -DSTVID_DEBUG_DECODER
endif

# General rules
ifeq "$(DVD_TOOLSET)" "ST40WINCE"
CFLAGS := -D__WINCE__ -D__SH4__ -DOSAL_OS21 -DHAL_ST40 -DMME20 $(VID_DEFS) $(CFLAGS)
else
CFLAGS := -D__OS21__ $(VID_DEFS) $(CFLAGS)
endif 
ifeq ($(STVID_NO_TRICKMODE),)
    ifneq ($(findstring $(DVD_BACKEND),"7109 7100 7200"),)
	CFLAGS := $(CFLAGS) -DST_speed
    endif
endif

# Modules
ifeq ($(findstring ordqueue,$(VID_OPTIONAL_MODULES)),ordqueue)
  CFLAGS += -D ST_ordqueue
  INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../ordqueue
endif

ifeq ($(findstring display,$(VID_OPTIONAL_MODULES)),display)
  CFLAGS += -D ST_display
  INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../display
endif

ifeq ($(findstring crc,$(VID_OPTIONAL_MODULES)),crc)
  CFLAGS += -D ST_crc
  INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../crc
endif

ifeq ($(findstring fgt,$(VID_OPTIONAL_MODULES)),fgt)
  INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../fgt
endif

# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../../.. $(DVD_BUILD_DIR)/../../../..
#ifdef STVID_VALID
# Current module: producer
CFLAGS += -D ST_producer
#endif

# Modules
    INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../api
    INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../buffers

    INCLUDE_PATH += $(DVD_BUILD_DIR)/preprocessor
    INCLUDE_PATH += $(RPC_ROOT)/include
    INCLUDE_PATH += $(RPC_ROOT)/src/mme/include
    INCLUDE_PATH += $(RPC_ROOT)/src/embx/include

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
h264decode.tco h264decode.o : h264decode.h
h264task.tco h264task.o : h264decode.h
h264dumpmme.tco h264dumpmme.o : h264dumpmme.h
h264decodecmd.tco h264decodecmd.o : h264dumpmme.h
#end of file
