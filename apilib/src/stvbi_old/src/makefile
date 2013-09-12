###################################
# MODULE : stvbi.lib / libstvbi.a #
###################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Define the targets
#-------------------
HEADER_IMPORTS := stos stvbi

ST20_IMPORTS := stdenc stpio stuart sttbx

ST20_TARGETS := stvbi.lib

ST20_OBJS := stvbi.tco



LINUX_IMPORTS := stcommon
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))

ST40_IMPORTS = $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_IMPORTS = $(ST20_IMPORTS)
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))



LINUX_OBJS := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))


# Define compilation options
#---------------------------
STVBI_SUBDIRS := hal hal/denc hal/viewport

ifeq ($(COPYPROTECTION), 1)
  VBI_DEFS := -D COPYPROTECTION
endif

ifeq ($(STVBI_DEBUG), 1)
  VBI_DEFS := -D STVBI_DEBUG
endif

#Activating MultiHandle Closed Caption
ifeq ($(VBI_MULT_CC), 1)
	VBI_DEFS := -D VBI_MULT_CC
endif
# Pass definitions to sub make calls
export VBI_DEFS

# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(STVBI_SUBDIRS))


# Sub-libs for HAL :
#--------------------------
SUBDIRS     := hal


# Apply compilation options
#--------------------------
CFLAGS := $(VBI_DEFS) $(CFLAGS)

# Pick up standard rules
#-----------------------
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
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
stvbi.tco stvbi.o: vbi_drv.h vbi_hal.h vbi_vpreg.h

