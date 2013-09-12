###############################################
# MODULE : vfe.lib / libvfe.a for stvtg       #
###############################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Add include path
#---------------------
INCLUDE_PATH += $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/../../..
INCLUDES +=  -I $(subst $(BAD_SLASH),$(GOOD_SLASH),../../../vos)
#----------------------------------------------------------------------
# set virtual paths to objects (as path from test application ./objs/ST20 subdirectory)
vpath %.c %.h ../../../vos

# Define the targets
#---------------------------------------------
HEADER_IMPORTS := stos stsys stvtg
ST20_IMPORTS := stevt stgxobj stpio stuart sttbx
ST200_IMPORTS := $(ST20_IMPORTS)

ST20_TARGETS := vfe.lib
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST20_OBJS := hal_vfe.tco clk_gen.tco
ST200_OBJS = $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))

LINUX_IMPORTS := stevt stgxobj stpio stcommon sttbx
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_OBJS = $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))

ST40_IMPORTS := $(ST20_IMPORTS)
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))


# Compile options
#----------------
CFLAGS := $(VTG_DEFS) $(CFLAGS)

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

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
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
hal_vfe.tco hal_vfe.o: vtg_drv.h vtg_hal2.h vfe_reg.h hal_vfe.h
