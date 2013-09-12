######################
# MODULE : video.lib #
######################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../..
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../src/api
ifeq "$(DVD_OS)" "LINUX"
INCLUDE_PATH +=  $(DVD_BUILD_DIR)/../../../src/linux
endif

# Define the targets and exported files
#--------------------------------------
ST20_TARGETS := video.lib
ifeq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
else
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
endif
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))


ST20_OBJS    := halv_lay.tco
ifeq "$(DVD_OS)" "LINUX"
ST40_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
else
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
endif
ST200_OBJS    := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))


IMPORTS := stos sttbx stevt stavmem stgxobj stvtg
ST40_IMPORTS := $(IMPORTS)

LINUX_IMPORTS := stcommon

# Sub-libs for each chips :
#--------------------------
ifeq "$(DVD_BACKEND)" "5510"
SUBDIRS     := omega1
endif
ifeq "$(DVD_BACKEND)" "5512"
SUBDIRS     := omega1
endif
ifeq "$(DVD_BACKEND)" "5508"
SUBDIRS     := omega1
endif
ifeq "$(DVD_BACKEND)" "5518"
SUBDIRS     := omega1
endif
ifeq "$(DVD_BACKEND)" "5514"
SUBDIRS     := omega1
endif
ifeq "$(DVD_BACKEND)" "5516"
SUBDIRS     := omega1
endif
ifeq "$(DVD_BACKEND)" "5517"
SUBDIRS     := omega1
endif
ifeq "$(DVD_BACKEND)" "5578"
SUBDIRS     := omega1
endif
ifeq "$(DVD_BACKEND)" "7015"
SUBDIRS     := omega2
endif
ifeq "$(DVD_BACKEND)" "7020"
SUBDIRS     := omega2
ifeq "$(DVD_FRONTEND)" "5528"
SUBDIRS     := sddispo2 $(SUBDIRS)
endif
endif
ifeq "$(DVD_BACKEND)" "GX1"
SUBDIRS     := # no sub-dir
endif
ifeq "$(DVD_BACKEND)" "5528"
SUBDIRS     := sddispo2
endif
ifeq "$(DVD_BACKEND)" "7710"
SUBDIRS     := sddispo2
endif
ifeq "$(DVD_BACKEND)" "7100"
SUBDIRS     := sddispo2
endif
ifeq "$(DVD_BACKEND)" "7109"
SUBDIRS     := displaypipe sddispo2
endif
ifeq "$(DVD_BACKEND)" "7200"
SUBDIRS     := displaypipe
ifdef STVID_USE_FGT
    SUBDIRS     += xvp
endif
endif
ifeq "$(DVD_BACKEND)" "DELTAPHI_HE"
SUBDIRS     := sddispo2
endif
ifeq "$(DVD_BACKEND)" "DELTAMU_HE"
SUBDIRS     := sddispo2
endif


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

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(LINUX_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif




# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning Video
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif




