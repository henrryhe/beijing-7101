# vi:noexpandtab
#######################################################################
#
# File Name: Makefile
#
# Description :
#     STSUBT component is the cooperation of several modules; each module is
#     implemented as a sub-directory into the source directory.
#
#     This is the master description file which invokes 'make' recursively 
#     in each sub-directory.
# 
# Copyright 2001 STMicroelectronics. All Rights Reserved.
# 
#
#######################################################################

DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

ST20_TARGETS := stsubt.lib
ST20_OBJS := stsubt.tco 

##zxg add for STPTI define
    CFLAGS := $(CFLAGS) -DDVD_TRANSPORT_STPTI
###
SUBDIRS := api filter decoder database display dispserv common

include $(DVD_MAKE)/defrules.mak

stsubt.lib:  $(SUBDIR_LIBS)
	$(ECHO) Building  $@
	$(BUILD_LIBRARY)


clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_$(TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

else
include $(DVD_MAKE)/builddir.mak
endif

