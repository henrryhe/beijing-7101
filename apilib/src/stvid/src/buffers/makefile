#########################
# MODULE : buffers.lib  #
#########################

# video specific variables
VID_MODULE_NAME = buffers
VID_OTHER_MODULES_DEPENDENCIES =
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
ST20_OBJS := vid_buff.tco
ST20_TARGETS := $(VID_MODULE_NAME).lib

# Sub-libs for HAL :
#--------------------------
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
vid_buff.tco vid_buff.o : ../vid_com.h vid_buff.h buffers.h hal/halv_buf.h

#end of file
