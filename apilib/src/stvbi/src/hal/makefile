#########################################
# MODULE : hal.lib / libhal.a for stvbi #
#########################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Add include path
#---------------------

ifeq "$(DVD_BACKEND)" "7710"
STVBI_HALSUBDIRS := .. denc viewport
else
ifeq "$(DVD_BACKEND)" "7109"
STVBI_HALSUBDIRS := .. denc viewport
else
ifeq "$(DVD_BACKEND)" "7100"
STVBI_HALSUBDIRS := .. denc viewport
else
ifeq "$(DVD_BACKEND)" "7200"
STVBI_HALSUBDIRS := .. denc viewport
else
STVBI_HALSUBDIRS := .. denc
endif
endif
endif
endif

INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(STVBI_HALSUBDIRS))

# Define the targets
#---------------------------------------------
HEADER_IMPORTS := stos stvbi stsys

ST20_IMPORTS := stpio stuart sttbx



ST20_TARGETS := hal.lib



ST20_OBJS := vbi_hal.tco



LINUX_IMPORTS := stcommon
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_IMPORTS := $(ST20_IMPORTS)
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

# Sub-libs :
#-------------------------------------------------
ifeq "$(DVD_BACKEND)" "7710"
        SUBDIRS     := denc  viewport
else
ifeq "$(DVD_BACKEND)" "7109"
        SUBDIRS     := denc  viewport
        else
ifeq "$(DVD_BACKEND)" "7100"
        SUBDIRS     := denc  viewport
        else
ifeq "$(DVD_BACKEND)" "7200"
        SUBDIRS     := denc  viewport
        else
        SUBDIRS     := denc
        endif
endif
endif
endif


# Apply compilation options
#--------------------------
CFLAGS := $(VBI_DEFS) $(CFLAGS)

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
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

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS) $(SUBDIR_LIBS)
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
vbi_hal.tco vbi_hal.o: vbi_drv.h vbi_hal.h ondenc.h


