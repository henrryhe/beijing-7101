# vi:noexpandtab
###############################################################################
# File : makefile
#
# Description:
#     Makefile of the STEVT component.
#
# Copyright 2001 STMicroelectronics. All Rights Reserved.
################################################################################
DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    ifeq "$(DVD_OS)" "OS40"
        HEADER_IMPORTS += os20emu
    endif
endif

ifdef STEVT_NO_PARAMETER_CHECK
CFLAGS += -DSTEVT_NO_PARAMETER_CHECK
endif

EXPORTS := stevt.h $(LIB_PREFIX)stevt$(LIB_SUFFIX)
TARGETS := $(LIB_PREFIX)stevt$(LIB_SUFFIX)

ifeq "$(DVD_OS)" "OS40"
ifndef STEVT_NO_TBX
IMPORTS := stcommon os20emu sttbx
else
IMPORTS := stcommon os20emu
endif

else
ifndef STEVT_NO_TBX
IMPORTS := stcommon sttbx
else
IMPORTS := stcommon
endif
endif

# ST40
ifndef ST40_TARGETS
ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
ST40_IMPORTS = $(IMPORTS)
endif

# ST200
ifndef ST200_TARGETS
ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
ST200_IMPORTS = $(IMPORTS)
endif

include $(DVD_MAKE)/defrules.mak

FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(OBJECT_DIRECTORY)

$(TARGETS): $(FULLDIR)/$(TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))

$(FULLDIR)/$(TARGETS): FORCE
	$(ECHO) Entering SRC directory
	$(MAKE) -C $(DVD_BUILD_DIR)/src

clean: subdir_clean
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_TARGETS)

subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean

FORCE:

else

include $(DVD_MAKE)/builddir.mak

endif

# End of sample ST20/ST40/ST200 makefile
