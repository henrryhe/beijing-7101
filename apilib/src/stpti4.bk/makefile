#########################################################################
# COPYRIGHT (C) SGS-THOMSON Microelectronics 2000
#
# Source file name : makefile
# Author           : Ralph Kimber
#
# Makefile to build stpti driver
#
# 28-5-2003     Update for PTI4 support.        GJP
# 23-7-2003     Refactored PTI4 support         GJP
# 14-9-2004     Update for OS21 support         PJW
#########################################################################

DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
ST20_EXPORTS := stpti4.h stpti4.lib
ST20_TARGETS := stpti4.lib

ST40_EXPORTS := stpti4.h libstpti4.a
ST40_TARGETS := libstpti4.a

ST200_EXPORTS := stpti4.h libstpti4.a
ST200_TARGETS := libstpti4.a

DVD_BUILD_DIR := $(DVD_BUILD_DIR)/src
SUBDIRS := stpti tcloaders

include $(DVD_MAKE)/defrules.mak

ifeq ($(STPTI_CONFIG_BSL),STPTI_BSL_DVB)
STPTI_CONFIG := -DSTPTI_DVB_SUPPORT -DSTPTI_BSL_SUPPORT -DSTPTI_NO_INDEX_SUPPORT -DSTPTI_NO_USAGE_CHECK
endif

ifeq ($(STPTI_CONFIG_BSL),STPTI_BSL_A3)
STPTI_CONFIG := -DSTPTI_DTV_SUPPORT -DSTPTI_DVB_SUPPORT -DSTPTI_BSL_SUPPORT -DSTPTI_NO_INDEX_SUPPORT -DSTPTI_NO_USAGE_CHECK
endif



ifdef NATIVE_COMPIL
$($(ARCHITECTURE)_TARGETS): libstpti.a libtccodenative.a
	$(BUILD_LIBRARY)
else
$($(ARCHITECTURE)_TARGETS):  $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif

clean: subdir_clean
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_TARGETS)

subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/stpti clean
	$(MAKE) -C $(DVD_BUILD_DIR)/tcloaders clean

FORCE:    
    
else

include $(DVD_MAKE)/builddir.mak

endif

loader:
	$(MAKE) -C src/tccode

loaders:
	$(MAKE) loader
	$(MAKE) header

header:
	$(ECHO) Creating stpti.h

ifdef NATIVE_COMPIL
	$(CP)  $(subst $(BAD_SLASH),$(GOOD_SLASH),src/tccode/$(LIB_PREFIX)tccode_c2$(LIB_SUFFIX)) $(LIB_PREFIX)tccodenative$(LIB_SUFFIX)
else
	$(MAKE) -C src/tccode tcproto.h
	$(CAT) $(subst $(BAD_SLASH),$(GOOD_SLASH),src/stpti/stptiint.h)  > stpti.h
	$(CAT) $(subst $(BAD_SLASH),$(GOOD_SLASH),src/tccode/tcproto.h) >> stpti.h
endif	

clean_loaders:
	$(RM) stpti.h
	$(MAKE) -C src/tccode clean

# ---  EOF ---
