##############################
# MODULE : crc.lib  #
##############################

# video specific variables
VID_MODULE_NAME = crc
VID_OTHER_MODULES_DEPENDENCIES =

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS    := vid_crc.tco

# Add specific definitions

ifdef VALID_TOOLS
     ST20_IMPORTS += valitls
else
     ifdef STVID_TRACE_BUFFER_UART
         INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../../tests/src
     endif
endif

ifneq ($(STVID_VIDCRC_TRACE_UART),)
# Define TRACE_VIDCRC to compile display with UART traces
	CFLAGS := $(CFLAGS) -D TRACE_VIDCRC
# Note : Set also STVID_TRACE_BUFFER_UART in order to link test application with trace.tco
endif

# Use trace toolbox instead of trace-uart (only info, error levels)
#------------------------------------------------------------------
CFLAGS += -D STTBX_PRINT

# compile with software CRC on platforms without hardware CRC IP
ifeq ($(findstring $(DVD_BACKEND),"7200 7109 7100 ZEUS DELTAMU_HE"),)
    CFLAGS += -D STVID_SOFTWARE_CRC_MODE
endif

# Pick up standars rules
#-----------------------
include $(DVD_BUILD_DIR)/../vidrules.mak

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
vid_crc.tco vid_crc.o : ../vid_com.h vid_crc.h crc.h

#end of file
