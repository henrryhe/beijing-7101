##########################
# MODULE : producer.lib  #
##########################

# video specific variables
VID_MODULE_NAME = producer
VID_OTHER_MODULES_DEPENDENCIES = buffers api
ifneq ($(findstring ordqueue,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) ordqueue
endif
ifneq ($(findstring inject,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) inject
endif
# decode may be needed for dual old legacy MPEG + producer & codecs build
ifneq ($(findstring decode,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) decode
endif
ifneq ($(findstring display,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) display
endif
ifneq ($(findstring fgt,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) fgt
endif



# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := vid_prod.tco producer.tco
ifdef STVID_CODEC_ONLY
  ST20_OBJS =
endif
ifdef STVID_VALID
  ST20_OBJS += val_prod.tco
endif

ifdef VALID_TOOLS
     ST20_IMPORTS += valitls
else
     ifdef STVID_TRACE_BUFFER_UART
         INCLUDE_PATH += $(DVD_BUILD_DIR)/../../tests/src
     endif
endif

ifneq ($(STVID_PRODUCER_TRACE_UART),)
# Define TRACE_PRODUCER to compile vid_prod.c with UART traces
	CFLAGS := $(CFLAGS) -DTRACE_PRODUCER
# Note : Set also STVID_TRACE_BUFFER_UART in order to link test application with trace.tco
endif

ifdef STVID_PRODUCER_NO_TIMEOUT
	CFLAGS := $(CFLAGS) -DSTVID_PRODUCER_NO_TIMEOUT
endif

ifdef STVID_DONT_SKIP_BADLY_PREPROCESSED_PICTURE
	CFLAGS := $(CFLAGS) -DSTVID_DONT_SKIP_BADLY_PREPROCESSED_PICTURE
endif

# Sub-libs for producer :
#------------------------
SUBDIRS := vidcodec

export VID_OPTIONAL_VIDCODECS

# Add definitions for codecs
CFLAGS += $(addprefix -DST_,$(VID_OPTIONAL_VIDCODECS))

# Pick up standars rules
#-----------------------
include $(DVD_BUILD_DIR)/../vidrules.mak

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
vid_prod.tco vid_prod.o : vid_prod.h vidcodec/vidcodec.h producer.h
producer.tco producer.o : producer.h vidcodec/vidcodec.h

#end of file
