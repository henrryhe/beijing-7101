##########################
# MODULE : ordqueue.lib  #
##########################

# video specific variables
VID_MODULE_NAME = ordqueue
VID_OTHER_MODULES_DEPENDENCIES = buffers
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
ST20_OBJS := vid_ordq.tco

# Pick up standard rules
#-----------------------
include $(DVD_BUILD_DIR)/../vidrules.mak

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
vid_ordq.tco vid_ordq.o : vid_ordq.h ordqueue.h

#end of file
