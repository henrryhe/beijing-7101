# vi:noexpandtab
##################################################################
#
# File Name: makefile
#
# Description :
#     STSUBT component is the cooperation of several modules; each module is
#     implemented as a sub-directory into the source directory.
#
#     This is the master description file which invokes 'make' recursively 
#     in each sub-directory.
# 
# Copyright 2000 STMicroelectronics. All Rights Reserved.
# 
#
######################################################################

DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

COMPONENT_NAME := stsubt

##zxg add for STPTI define
    CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_STPTI
###
ST20_TARGETS := $(COMPONENT_NAME).lib

ST20_EXPORTS := $(COMPONENT_NAME).h $(COMPONENT_NAME).lib

include $(DVD_MAKE)/defrules.mak

FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(ARCHITECTURE)

$(ST20_TARGETS): $(FULLDIR)/$(ST20_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))

$(FULLDIR)/$(ST20_TARGETS): FORCE
	$(ECHO) Entering SRC directory
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


