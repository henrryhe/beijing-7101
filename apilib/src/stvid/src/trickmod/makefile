##########################
# MODULE : trickmod.lib  #
##########################

# video specific variables
VID_MODULE_NAME = trickmod
VID_OTHER_MODULES_DEPENDENCIES = buffers producer
ifneq ($(findstring ordqueue,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) ordqueue
endif
ifneq ($(findstring decode,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) decode
endif
ifneq ($(findstring display,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) display
endif
ifneq ($(findstring crc,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) crc
endif
ifneq ($(findstring fgt,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) fgt
endif
ifneq ($(findstring inject,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) inject
endif
ifneq ($(findstring diginput,$(VID_OPTIONAL_MODULES)),)
VID_OTHER_MODULES_DEPENDENCIES += diginput
endif

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := vid_tric.tco

# if debug compilation selected, Uncomment line below to put breakpoint on Debug error function TrickModeError
# CFLAGS := $(CFLAGS) -D TRICKMOD_DEBUG
# CFLAGS := $(CFLAGS) -D PARSE_AND_DECODE_EXCLUSION_TEST
# CFLAGS := $(CFLAGS) -D LOOP_OPTIMISATION

# Pick up standars rules
#-----------------------
include $(DVD_BUILD_DIR)/../vidrules.mak

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
vid_tric.tco vid_tric.o : ../vid_com.h vid_tric.h

#end of file
