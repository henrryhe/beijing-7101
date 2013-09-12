##############################
# MODULE : fgt.lib  #
##############################

# video specific variables
VID_MODULE_NAME = fgt
VID_OTHER_MODULES_DEPENDENCIES = 

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS    := vid_fgt.tco

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
	CFLAGS := $(CFLAGS) -D TRACE_VIDFGT
# Note : Set also STVID_TRACE_BUFFER_UART in order to link test application with trace.tco
endif

# Use trace toolbox instead of trace-uart (only info, error levels)
#------------------------------------------------------------------
CFLAGS += -D STTBX_PRINT

# 
#------------------------------------------------------------------
ifdef STVID_FGT_LOAD_DATABASE
  CFLAGS := $(CFLAGS) -DSTVID_FGT_LOAD_DATABASE='"$(STVID_FGT_LOAD_DATABASE)"'
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
vid_fgt.tco vid_fgt.o : ../vid_com.h vid_fgt.h vid_fgt_data.h

#end of file
