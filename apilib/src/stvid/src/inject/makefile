#######################
# MODULE : inject.lib #
#######################

# video specific variables
VID_MODULE_NAME = inject
VID_OTHER_MODULES_DEPENDENCIES =

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ifdef STUB_INJECT
ST20_OBJS := inject_stub.tco
else
ST20_OBJS := inject.tco
endif

# Pick up standard rules
#-----------------------
include $(DVD_BUILD_DIR)/../vidrules.mak

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
inject.tco inject.o : ../vid_com.h inject.h
inject_stub.tco inject_stub.o : ../vid_com.h inject.h

#end of file
