############################
# MODULE : parser.lib  #
############################

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := mpg2dec.tco
ifdef STUB_FMW_ST40
    ST20_OBJS+= mpg2stub.tco
endif

ifdef STVID_VALID
  ST20_OBJS += mpg2dumpmme.tco
endif


ST20_TARGETS := decoder.lib
ST20_IMPORTS := sttbx stevt stos
ifdef VALID_TOOLS
     ST20_IMPORTS += valitls
else
     ifdef STVID_TRACE_BUFFER_UART
         INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../../tests/src
     endif
endif

ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(ST20_EXPORTS)
ST40_IMPORTS := $(ST20_IMPORTS)

ifneq ($(STVID_DECODER_TRACE_UART),)
# Define TRACE_DECODER to compile mpg2decoder with UART traces
	CFLAGS := $(CFLAGS) -D TRACE_DECODER
# Note : Set also STVID_TRACE_BUFFER_UART in order to link test application with trace.tco
endif

# General rules
CFLAGS := $(VID_DEFS) $(CFLAGS)

ifeq ($(STVID_NO_TRICKMODE),)
    ifneq ($(findstring $(DVD_BACKEND),"7109 7200"),)
	CFLAGS := $(CFLAGS) -DST_speed
    endif
endif

ifdef MME_DUMP
	CFLAGS := $(CFLAGS) -DMME_DUMP
endif

ifneq ($(STUB_FMW_ST40),)
    CFLAGS := $(CFLAGS) -DSTUB_FMW_ST40
endif

ifdef STVID_VALID
	CFLAGS := $(CFLAGS) -DST_producer
endif


# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../../.. $(DVD_BUILD_DIR)/../../../..
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../inject
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../api
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../buffers
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../ordqueue
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../display
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../fgt
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
	@$(ECHO) Cleaning MPG2 Video CODEC decoder $(ST20_TARGETS)

clean: clean_message $(SUBDIR_CLEAN)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

FORCE:


# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
mpg2dec.tco mpg2dec.o : ./mpg2dec.h ../vcodmpg2.h ./MPEG2_VideoTransformerTypes.h
mpg2stub.tco mpg2stub.o : ./mpg2dec.h ../vcodmpg2.h ./MPEG2_VideoTransformerTypes.h
mpg2dumpmme.tco mpg2dumpmme.o : ./mpg2dec.h ../vcodmpg2.h ./MPEG2_VideoTransformerTypes.h mpg2dumpmme.h
#end of file
