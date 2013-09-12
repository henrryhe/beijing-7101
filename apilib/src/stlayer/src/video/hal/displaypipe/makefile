############################
# MODULE : displaypipe.lib #
############################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

ST40_IMPORTS := stos

# Concat include path
#---------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/..
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../../src/api
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../../src

# Local objects which comprise this component
#---------------------------------------------
ST20_TARGETS := displaypipe.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

ST20_OBJS := hv_vdplay.tco hv_vdptools.tco hv_vdpdei.tco hv_vdpfmd.tco
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

LINUX_IMPORTS := stcommon

ifneq ($(VIDEO_MINIMIZE_MEMORY_USAGE_FOR_DEI),)
  CFLAGS := $(CFLAGS) -DSTLAYER_MINIMIZE_MEMORY_USAGE_FOR_DEI
endif

ifneq ($(STLAYER_NO_DEINTERLACER),)
  CFLAGS := $(CFLAGS) -DSTLAYER_NO_DEINTERLACER
endif

# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS)
	$(BUILD_LIBRARY)
endif

# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning DisplayPipe
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Dependencies
hv_vdptools.tco hv_vdptools.o : hv_vdpfilt.h

