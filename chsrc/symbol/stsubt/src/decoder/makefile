# vi:noexpandtab
#######################################################################
# File : makefile
#
# Description :
#       Makefile to build a subdirectory
#
# Copyright 2001 STMicroelectronics. All Rights Reserved.
#
# Author : Nunzio Raciti
#######################################################################

DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS += stsubt

IMPORTS := stpti stgpdma

CFLAGS += $(OPTIONAL_CFLAGS) 

ifdef PTI_EMULATION_MODE
CFLAGS += -DPTI_EMULATION_MODE
endif

ifdef RDE_DISPLAY_SERVICE
CFLAGS += -DRDE_DISPLAY_SERVICE
CFLAGS += -DDISABLE_OSD_DISPLAY_SERVICE
endif

SUBT_COMPONENT    := $(notdir $(DVD_BUILD_DIR))

ST20_TARGETS := $(SUBT_COMPONENT).lib
ST20_OBJS    := decoder.tco clut.tco pcs.tco obj.tco pts.tco rcs.tco eds.tco

PATH_SRC_DIR := $(subst $(GOOD_SLASH)$(SUBT_COMPONENT),,$(DVD_BUILD_DIR))

INCLUDE_PATH := $(PATH_SRC_DIR)/api $(PATH_SRC_DIR)/filter $(PATH_SRC_DIR)/decoder $(PATH_SRC_DIR)/display $(PATH_SRC_DIR)/dispserv $(PATH_SRC_DIR)/common $(PATH_SRC_DIR)/database $(INCLUDE_PATH)

include $(DVD_MAKE)/defrules.mak

$(SUBT_COMPONENT).lib: $(ST20_OBJS)
	@$(ECHO) Building  $@
	$(BUILD_LIBRARY)

clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS) 

else

include $(DVD_MAKE)/builddir.mak

endif


