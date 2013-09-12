###################################################
# MODULE : viewport.lib / libviewport.a for stvbi #
###################################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Add include path
#---------------------
INCLUDE_PATH += $(DVD_BUILD_DIR)/../..

# Define the targets
#-------------------
HEADER_IMPORTS := stos stvbi

ST20_IMPORTS := stcommon stpio stuart sttbx
ST40_IMPORTS := $(ST20_IMPORTS)
ST200_IMPORTS = $(ST20_IMPORTS)
LINUX_IMPORTS := stcommon


ST20_TARGETS := viewport.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))


ST20_OBJS := vbi_vpfullcgms.tco
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))


# Apply compilation options
#--------------------------
CFLAGS := $(VBI_DEFS) $(CFLAGS)

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
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
	-$(RM) $($(BUILDING_FOR)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
STVBI_DEPEND := vbi_drv.h
vbi_vpfullcgms.tco vbi_vpfullcgms.o: $(STVBI_DEPEND)
