#########################
# MODULE : sddispo2.lib #
#########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

ST40_IMPORTS := stos
ST20_IMPORTS := stos
ST200_IMPORTS := stos

# Concat include path
#---------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/..
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../../src/api
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../../src

# Local objects which comprise this component
#---------------------------------------------
ST20_TARGETS := sddispo2.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))

ST20_OBJS := hv_lay8.tco hv_tools.tco
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST200_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

LINUX_IMPORTS := stcommon

ifeq "$(STI7710_CUT2x)" "TRUE"
DVD_CFLAGS := $(DVD_CFLAGS) -DSTI7710_CUT2x
endif

ifeq "$(DISABLE_LMU)" "TRUE"
DVD_CFLAGS := $(DVD_CFLAGS) -DDISABLE_LMU
endif


# Pick up standars rules
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

ifeq "$(BUILDING_FOR)" "ST200"





$(ST200_TARGETS): $(ST200_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS)
	$(BUILD_LIBRARY)
endif


# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning Sddispo2
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Dependencies
hv_tools.tco hv_tools.o : hv_filt8.h hv_lay8.h

