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
ST20_EXPORTS = $(COMPONENT_NAME).h $(COMPONENT_NAME).lib

ST40_IMPORTS := os20emu  
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))

include $(DVD_MAKE)/defrules.mak

FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(OBJECT_DIRECTORY)

$(ST20_TARGETS): $(FULLDIR)/$(ST20_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))

$(ST40_TARGETS): $(FULLDIR)/$(ST40_TARGETS) 
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))

$(FULLDIR)/$(ST20_TARGETS): FORCE
	$(ECHO) Entering SRC directory
	$(MAKE) -C $(DVD_BUILD_DIR)/src

$(FULLDIR)/$(ST40_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src
    

clean: subdir_clean
	$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_TARGETS)

subdir_clean: 
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean

FORCE:

else

include $(DVD_MAKE)/builddir.mak

endif
