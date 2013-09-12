##########################
# MODULE : transcoder.lib  #
##########################

# video specific variables
VID_MODULE_NAME = transcoder
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
ifneq ($(findstring producer,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) producer
endif

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Sub-libs for producer :
#------------------------
SUBDIRS := transcodec

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := vid_tran.tco vid_trqueue.tco transcoder.tco
ifdef STVID_CODEC_ONLY
  ST20_OBJS =
endif

ifdef VALID_TOOLS
     ST20_IMPORTS += valitls
else
     ifdef STVID_TRACE_BUFFER_UART
         INCLUDE_PATH += $(DVD_BUILD_DIR)/../../tests/src
     endif
endif

# Pick up standards rules
#-----------------------
include $(DVD_BUILD_DIR)/../vidrules.mak

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependencies
vid_tran.tco vid_tran.o : vid_tran.h vid_trqueue.h transcoder.h
vid_trqueue.tco vid_trqueue.o : vid_trqueue.h
transcoder.tco transcoder.o : transcoder.h 

#end of file
