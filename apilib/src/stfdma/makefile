# vi:noexpandtab
##################################################################
#
# File Name: makefile
#
# Description : Top level STFDMA makefile
# 
# Copyright 2002 STMicroelectronics. All Rights Reserved.
# 
#
######################################################################

DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

COMPONENT_NAME := stfdma

ST20_IMPORTS := stos
ST40_IMPORTS := $(ST20_IMPORTS)
ST200_IMPORTS := $(ST20_IMPORTS)

ST20_TARGETS := $(COMPONENT_NAME).lib
ST20_EXPORTS := $(COMPONENT_NAME).h $(COMPONENT_NAME).lib

ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))

ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))

include $(DVD_MAKE)/defrules.mak

ifdef STFDMA_CALLBACK_TASK_PRIORITY
  CFLAGS += -DSTFDMA_CALLBACK_TASK_PRIORITY=$(STFDMA_CALLBACK_TASK_PRIORITY)
endif

FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(OBJECT_DIRECTORY)

$(ST20_TARGETS): $(FULLDIR)/$(ST20_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))

$(FULLDIR)/$(ST20_TARGETS): FORCE
	$(ECHO) Entering SRC directory
	$(MAKE) -C $(DVD_BUILD_DIR)/src

ifeq ($(ARCHITECTURE), ST40)
$(ST40_TARGETS): $(FULLDIR)/$(ST40_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))

$(FULLDIR)/$(ST40_TARGETS): FORCE
	$(ECHO) Entering SRC directory
	$(MAKE) -C $(DVD_BUILD_DIR)/src
endif
        
ifeq ($(ARCHITECTURE), ST200)
$(ST200_TARGETS): $(FULLDIR)/$(ST200_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))

$(FULLDIR)/$(ST200_TARGETS): FORCE
	$(ECHO) Entering SRC directory
	$(MAKE) -C $(DVD_BUILD_DIR)/src
endif

clean: subdir_clean
	$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_TARGETS)

subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean

FORCE:

else

include $(DVD_MAKE)/builddir.mak

endif


