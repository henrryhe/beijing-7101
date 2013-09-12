#########################
# MODULE : display.lib  #
#########################

# video specific variables
VID_MODULE_NAME = display
VID_OTHER_MODULES_DEPENDENCIES = buffers
ifneq ($(findstring ordqueue,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) ordqueue
endif
ifneq ($(findstring crc,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) crc
endif
ifneq ($(findstring fgt,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) fgt
endif

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS    := vid_disp.tco vid_queu.tco vid_sets.tco \
                vid_flds.tco vid_cmd.tco  vid_dini.tco

# Add specific definitions

# Add debug traces : uncomment to set
#------------------------------------
#CFLAGS += -D TRACE_CMD
#CFLAGS += -D TRACE_INIT
#CFLAGS += -D TRACE_DISPLAY
#CFLAGS += -D TRACE_FIELDS
#CFLAGS += -D TRACE_SETTING
#CFLAGS += -D TRACE_QUEUE
#CFLAGS += -D TRACE_FIELDS_VERBOSE
#CFLAGS += -D TRACE_QUEUE_VERBOSE
#CFLAGS += -D TRACE_DISPLAY_VERBOSE
#CFLAGS += -D TRACE_FRAME_RATE_CONVERSION

# Use trace toolbox instead of trace-uart (only info, error levels)
#------------------------------------------------------------------
#CFLAGS += -D STTBX_PRINT

# Sub-libs for HAL :
#-------------------
SUBDIRS     := hal

# Pick up standars rules
#-----------------------
include $(DVD_BUILD_DIR)/../vidrules.mak

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
vid_disp.tco vid_disp.o : ../vid_com.h vid_disp.h display.h queue.h fields.h hal/halv_dis.h
vid_queu.tco vid_queu.o : ../vid_com.h vid_disp.h display.h queue.h
vid_sets.tco vid_sets.o : ../vid_com.h vid_disp.h display.h ../vid_ctcm.h
vid_flds.tco vid_flds.o : ../vid_com.h vid_disp.h display.h fields.h
vid_cmd.tco  vid_cmd.o  : ../vid_com.h vid_disp.h display.h queue.h fields.h ../vid_ctcm.h hal/halv_dis.h
vid_dini.tco vid_dini.o : ../vid_com.h vid_disp.h display.h queue.h ../vid_ctcm.h hal/halv_dis.h

#end of file
