#########################
# MODULE : stblit.lib #
#########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Define the targets and exported files
#--------------------------------------
ST20_TARGETS := stblit.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))


# Conditional rules to generate the graphic layer lib (graphics.lib)
# The sub drivers used depend on the chip
#-------------------------------------------------------------------
ifneq (,$(findstring $(DVD_BACKEND),5508 5518 5510 5512 5514 5516 5517))
BLT_DEFS = -I$(DVD_BUILD_DIR)/blt_emu -I$(DVD_BUILD_DIR)/blt_gam -DSTBLIT_EMULATOR
SUBDIRS := blt_gam blt_emu
endif

ifneq (,$(findstring $(DVD_BACKEND),7015 7020 GX1 5528 7710 7100))
SUBDIRS := blt_gam
ifeq "$(STBLIT_SOFTWARE)" "TRUE"
BLT_DEFS = -I$(DVD_BUILD_DIR)/blt_emu -I$(DVD_BUILD_DIR)/blt_gam -DSTBLIT_EMULATOR
SUBDIRS := blt_gam blt_emu
else
ifeq "$(STBLIT_EMULATOR)" "TRUE"
BLT_DEFS = -I$(DVD_BUILD_DIR)/blt_emu -I$(DVD_BUILD_DIR)/blt_gam -DSTBLIT_EMULATOR
SUBDIRS := blt_gam blt_emu
endif
endif
endif


ifneq (,$(findstring $(DVD_BACKEND),5100 5105 5188 5301 5525 5107 7109 7200))
SUBDIRS := blt_bdisp
ifeq "$(STBLIT_SOFTWARE)" "TRUE"
BLT_DEFS = -I$(DVD_BUILD_DIR)/blt_emu -I$(DVD_BUILD_DIR)/blt_gam -DSTBLIT_EMULATOR
SUBDIRS := blt_gam blt_emu
else
ifeq "$(STBLIT_EMULATOR)" "TRUE"
BLT_DEFS = -I$(DVD_BUILD_DIR)/blt_emu -I$(DVD_BUILD_DIR)/blt_gam -DSTBLIT_EMULATOR
SUBDIRS := blt_gam blt_emu
endif
endif
endif


ifeq "$(ZEUS_BACKEND)" "7109"
SUBDIRS := blt_bdisp
ifeq "$(STBLIT_SOFTWARE)" "TRUE"
BLT_DEFS = -I$(DVD_BUILD_DIR)/blt_emu -I$(DVD_BUILD_DIR)/blt_gam -DSTBLIT_EMULATOR
SUBDIRS := blt_gam blt_emu
else
ifeq "$(STBLIT_EMULATOR)" "TRUE"
BLT_DEFS = -I$(DVD_BUILD_DIR)/blt_emu -I$(DVD_BUILD_DIR)/blt_gam -DSTBLIT_EMULATOR
SUBDIRS := blt_gam blt_emu
endif
endif
endif


ifeq "$(STBLIT_USE_MEMORY_TRACE)" "TRUE"
BLT_DEFS += -I $(DVD_BUILD_DIR)/blt_utils
SUBDIRS += blt_utils
endif


# export variable to sub dirs
export BLT_DEFS

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build the sub-libs
#-------------------
$(ST20_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)


ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif
ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif
ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif


# Delete all objects and libs
#----------------------------
clean: $(SUBDIR_CLEAN)
	$(RM) $($(OBJECT_DIRECTORY)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

