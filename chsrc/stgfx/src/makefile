# vi:noexpandtab
################################################################################
#
# File Name: Makefile
#
# Description :
#     STGFX component is the cooperation of several modules; each module is
#     implemented as a sub-directory into the source directory.
#
#     This is the master description file which invokes 'make' recursively 
#     in each sub-directory.
# 
# Copyright 2000 STMicroelectronics. All Rights Reserved.
# 
#
################################################################################

DVD_MAKE_VERSION :=2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

COMPONENT_NAME := stgfx

ST20_TARGETS = $(COMPONENT_NAME).lib

ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_IMPORTS := os20emu

SUBDIRS := gfx
ifdef OSD_WRAPPER
SUBDIRS += blit
endif

# Scalability support
include $(DVD_BUILD_DIR)/scaling.mak

ifdef STGFX_INCLUDE_TEXT
SUBDIRS += bdf olf txt
endif

include $(DVD_MAKE)/defrules.mak

$($(ARCHITECTURE)_TARGETS): $(SUBDIR_LIBS)
ifeq "NOT" "EQUAL" # Debug (comes out after dependencies compiled)
	$(ECHO) STGFX_OPTIONS: $(STGFX_OPTIONS)
	$(ECHO) STGFX_INCLUDE_LINES:      $(STGFX_INCLUDE_LINES)
	$(ECHO) STGFX_INCLUDE_ARCS:       $(STGFX_INCLUDE_ARCS)
	$(ECHO) STGFX_INCLUDE_TEXT:       $(STGFX_INCLUDE_TEXT)
	$(ECHO) STGFX_INCLUDE_COPY:       $(STGFX_INCLUDE_COPY)
	$(ECHO) STGFX_INCLUDE_FLOODFILL:  $(STGFX_INCLUDE_FLOODFILL)
endif
	$(ECHO) Building $@ 
	$(BUILD_LIBRARY)

clean: $(SUBDIR_CLEAN)
	$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_TARGETS)

FORCE:

else

include $(DVD_MAKE)/builddir.mak

endif
