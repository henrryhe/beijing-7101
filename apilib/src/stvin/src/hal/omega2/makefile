########################
# MODULE : omega2.lib  #
########################
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
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../..

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := hv_hd_in.tco hv_sd_in.tco hv_dvp.tco
ST20_TARGETS := omega2.lib

ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))

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
	echo Cleaning omega2.lib
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)


# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

hv_sd_in.o hv_sd_in.tco: \
 hv_sd_in.c \
 stddefs.h \
 stlite.h \
 hv_sd_in.h \
 ../halv_vin.h \
 stvin.h \
 stgxobj.h \
 stavmem.h \
 stevt.h \
 hv_reg.h \
 stsys.h \
 sttbx.h \
 stpio.h \
 stuart.h

hv_hd_in.o hv_hd_in.tco : \
 hv_hd_in.c \
 stddefs.h \
 stlite.h \
 hv_hd_in.h \
 ../halv_vin.h \
 stvin.h \
 stgxobj.h \
 stavmem.h \
 stevt.h \
 hv_reg.h \
 stsys.h \
 sttbx.h \
 stpio.h \
 stuart.h

hv_dvp.o hv_dvp.tco : \
 hv_dvp.c \
 stddefs.h \
 stlite.h \
 hv_dvp.h \
 ../halv_vin.h \
 stvin.h \
 stgxobj.h \
 stavmem.h \
 stevt.h \
 hv_reg.h \
 stsys.h \
 sttbx.h \
 stpio.h \
 stuart.h








