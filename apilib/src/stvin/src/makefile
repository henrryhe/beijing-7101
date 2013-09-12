########################
# MODULE : stvin.lib #
########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    ifeq "$(DVD_OS)" "OS40"
        HEADER_IMPORTS += os20emu
    endif
endif

# Define the targets and exported files
#--------------------------------------
ST20_TARGETS := stvin.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

ST20_OBJS   := vin_init.tco vin_cmd.tco vin_ctcm.tco vin_inte.tco
ST40_OBJS   := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
LINUX_OBJS  := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

STPTILT_DEVICES = 7710
ifeq "$(findstring $(DVD_FRONTEND), $(STPTILT_DEVICES))" "$(DVD_FRONTEND)"
STPTI_VOB_NAME = $(DVD_TRANSPORT)lt
else
STPTI_VOB_NAME = $(DVD_TRANSPORT)
endif

ST20_IMPORTS := $(STPTI_VOB_NAME) stos
ST40_IMPORTS := $(ST20_IMPORTS)
LINUX_IMPORTS := $(ST20_IMPORTS)

# Activate Clock Recovery
#------------------------
ifneq (,$(findstring $(DVD_BACKEND),7015 7020))
CFLAGS += -D ST_stclkrv
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS += -DSTAPI_INTERRUPT_BY_NUMBER
endif

# Sub-libs for HAL :
#--------------------------
SUBDIRS     := hal

# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/..

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	echo Cleaning stvin.lib
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local Dependencies
#-------------------
vin_cmd.tco: vin_cmd.c \
 stvin.h \
 stddefs.h \
 stlite.h \
 stgxobj.h \
 stavmem.h \
 stevt.h \
 sttbx.h \
 stpio.h \
 stuart.h \
 vin_drv.h \
 vin_init.h \
 hal/halv_vin.h

vin_init.tco: vin_init.c \
 stddefs.h \
 stlite.h \
 sttbx.h \
 stpio.h \
 stuart.h \
 stvin.h \
 stgxobj.h \
 stavmem.h \
 stevt.h \
 vin_init.h \
 hal/halv_vin.h \
 vin_drv.h
