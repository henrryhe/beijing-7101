########################
# MODULE : laycompo.lib #
########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

IMPORTS := stos compo stdisp

# Concat include path
#---------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../..
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../api

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := lc_vp.tco lc_init.tco lc_pool.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

ST20_TARGETS := laycompo.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST200_IMPORTS := os20emu compo stdisp

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

# Clean .lib and obj
#-------------------
clean:
	@$(ECHO) Cleaning Gamma
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

