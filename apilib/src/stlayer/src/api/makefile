####################
# MODULE : api.lib #
####################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak


ST40_IMPORTS := stos
ST20_IMPORTS := stos
ST200_IMPORTS := stos

# Concat include path
#--------------------
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../..
ifneq (,$(findstring $(DVD_BACKEND),5100 5105 5525 5188 5301 5107 5162))
    INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../laycompo/
endif
ifdef STVID_USE_FGT
    INCLUDE_PATH += $(DVD_BUILD_DIR)/../video/hal/xvp
endif
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../video/hal
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../graphics/hal/gamma
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../graphics/hal/osdframe
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../graphics/hal/osdtopbt
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../graphics/hal/stillpic
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../graphics/hal/sub_pict
ifeq ($(ARCHITECTURE),LINUX)
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../linux
endif
# Add optional Define 'OLD_ARCHI' (= graphics are not linked)
#------------------------------------------------------------
ifeq  "$(DVD_OLD_GFX_API)" "TRUE"
CFLAGS :=  -DOLD_ARCHI  $(CFLAGS)
endif

# local objects which comprise this component
#--------------------------------------------
ST20_OBJS = layervp.tco layerlay.tco layerflt.tco
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

ST20_TARGETS = api.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))


LINUX_IMPORTS := stcommon

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the library
#------------------
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

# Delete all objects and libs
#-----------------------------
clean:
	@$(ECHO) Cleaning api
	$(RM) $($(BUILDING_FOR)_OBJS)
	$(RM) $($(BUILDING_FOR)_TARGETS)

# Add for make system version 2 support
else
include $(DVD_MAKE)/builddir.mak
endif

