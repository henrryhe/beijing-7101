#########################
# MODULE : hal.lib  #
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
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/..

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := halv_vin.tco
ST20_TARGETS := hal.lib

ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := ST20_IMPORTS

STPTILT_DEVICES = 7710
ifeq "$(findstring $(DVD_FRONTEND), $(STPTILT_DEVICES))" "$(DVD_FRONTEND)"
STPTI_VOB_NAME = $(DVD_TRANSPORT)lt
else
STPTI_VOB_NAME = $(DVD_TRANSPORT)
endif

ST20_IMPORTS := $(STPTI_VOB_NAME) stos
ST40_IMPORTS := $(ST20_IMPORTS) os20emu

LINUX_IMPORTS := $(ST20_IMPORTS)
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

# compil flags, objs and clean_cell for each chips
#-------------------------------------------------
# Sub-libs for each chips :
#--------------------------
ifeq "$(DVD_BACKEND)" "7015"
SUBDIRS     := omega2
endif
ifeq "$(DVD_BACKEND)" "7020"
SUBDIRS     := omega2
endif
ifeq "$(DVD_BACKEND)" "GX1"
SUBDIRS     := omega2
endif
ifeq "$(DVD_BACKEND)" "5528"
SUBDIRS     := omega2
endif
ifeq "$(DVD_BACKEND)" "7710"
SUBDIRS     := omega2
endif
ifeq "$(DVD_BACKEND)" "7100"
SUBDIRS     := omega2
endif
ifeq "$(DVD_BACKEND)" "7109"
SUBDIRS     := omega2
endif

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	echo Cleaning hal.lib
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

halv_vin.tco: \
 halv_vin.c \
 stddefs.h \
 stlite.h \
 stvin.h \
 stgxobj.h \
 stavmem.h \
 stevt.h \
 halv_vin.h \
 omega2/hv_sd_in.h \
 omega2/hv_hd_in.h \
 omega2/hv_dvp.h \
 sttbx.h \
 stpio.h \
 stuart.h
