#########################
# MODULE : gamma.lib #
#########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    ifeq "$(DVD_OS)" "OS40"
        HEADER_IMPORTS += os20emu
    endif
endif


# Concat include path
#---------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../..
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../api

ifeq "$(BUILDING_FOR)" "LINUX"
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../..
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../linux
endif


# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := gfxutils.tco pool_mng.tco hard_mng.tco front_vp.tco back_end.tco \
             front_cm.tco desc_mng.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

ST20_TARGETS := gamma.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

ST40_IMPORTS := os20emu stos stavmem
ST20_IMPORTS := os20emu stos
ST200_IMPORTS := os20emu stos
LINUX_IMPORTS := stcommon

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

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS)
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

