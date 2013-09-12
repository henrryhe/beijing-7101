#####################
# MODULE : api.lib  #
#####################

# video specific variables
VID_MODULE_NAME = api
VID_OTHER_MODULES_DEPENDENCIES = buffers ordqueue $(VID_OPTIONAL_MODULES)

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := vid_com.tco vid_deco.tco vid_pict.tco vid_err.tco vid_buf.tco vid_dvpw.tco

ifneq ($(findstring composition,$(VID_OPTIONAL_FEATURES)),)
ST20_OBJS := $(ST20_OBJS) vid_comp.tco
endif
ifneq ($(findstring crc,$(VID_OPTIONAL_MODULES)),)
  ST20_OBJS += vid_crcapi.tco
endif

ifneq ($(findstring transcoder,$(VID_OPTIONAL_MODULES)),)
ST20_OBJS := $(ST20_OBJS) vid_xcode.tco
endif

# Pick up standars rules
#-----------------------
include $(DVD_BUILD_DIR)/../vidrules.mak

INCLUDE_PATH += ../../src

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
vid_com.tco vid_com.o   : ../vid_com.h ../vid_ctcm.h api.h vid_err.h
vid_comp.tco vid_comp.o : ../vid_com.h ../vid_ctcm.h api.h vid_err.h
vid_dvpw.tco vid_dvpw.o : ../vid_com.h ../vid_ctcm.h api.h vid_err.h
vid_deco.tco vid_deco.o : ../vid_com.h ../vid_ctcm.h api.h vid_err.h
vid_pict.tco vid_pict.o : ../vid_com.h ../vid_ctcm.h api.h
vid_err.tco vid_err.o   : ../vid_com.h ../vid_ctcm.h api.h vid_err.h
vid_crcapi.tco vid_crcapi.o   : ../vid_com.h ../vid_ctcm.h api.h vid_err.h
vid_xcode.tco vid_xcode.o   : ../vid_com.h ../vid_ctcm.h api.h vid_xcode.h

#end of file
