###############################################
# MODULE : source.lib / libsource.a for sthdmi#
###############################################

# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Add include path
#---------------------
INCLUDE_PATH += $(DVD_BUILD_DIR)/..

# Define the targets
#---------------------------------------------
HEADER_IMPORTS := stos stsys sthdmi staudlx stclkrv $(DVD_TRANSPORT)
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

ST20_IMPORTS := stpio stuart sttbx
ST20_TARGETS := source.lib
ST20_OBJS := hdmi_src.tco

ST40_IMPORTS := $(ST20_IMPORTS) os20emu
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

LINUX_IMPORTS := $(ST20_IMPORTS)
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))


# Check if HDMI audio workaround enabled
#---------------------------------------
ifdef WA_GNBvd44290
CFLAGS += -DWA_GNBvd44290
endif

ifdef WA_GNBvd54182
CFLAGS += -DWA_GNBvd54182
endif


# Compile options
#----------------
CFLAGS := $(CFLAGS) -DSTTBX_PRINT -DSTTBX_INPUT

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
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
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
hdmi_src.tco : hdmi_src.h hdmi_drv.h

