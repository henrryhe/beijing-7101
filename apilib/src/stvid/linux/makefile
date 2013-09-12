# COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

.PHONY: stvid_tests


LINUX_TARGETS := 

ifdef STVID_BUILD_TESTS
LINUX_TARGETS += stvidtest_core.ko stvidtest_ioctl.ko libstvidtest.a 
endif

LINUX_TARGETS += stvid_core.ko stvid_ioctl.ko libstvid.a


include $(DVD_MAKE)/defrules.mak

# For building kernel objects
$(filter %.ko,$(LINUX_TARGETS)): FORCE
	@$(ECHO) Building $@
	-$(CP) $(DVD_MAKE)/Modules.symvers $(DVD_BUILD_DIR)/$(basename $@)/.
	@$(MAKE) -C $(DVD_BUILD_DIR)/$(basename $@)
	@$(CP) $(DVD_BUILD_DIR)/$(basename $@)/$@ .
	-$(RENAME) $(DVD_BUILD_DIR)/$(basename $@)/Modules.symvers $(DVD_MAKE)/.

# For building libraries
$(filter lib%.a,$(LINUX_TARGETS)): FORCE
	@$(ECHO) Building $@
	@$(MAKE) -C $(DVD_BUILD_DIR)/$(patsubst lib%.a,%_ioctl,$@) $@
	@$(CP) $(DVD_BUILD_DIR)/$(patsubst lib%.a,%_ioctl,$@)/$@ .

FORCE:

stvid_tests:
	@$(ECHO) Building all LINUX tests
	@$(MAKE) -C $(DVD_BUILD_DIR)/test
	@$(CP) `find $(DVD_BUILD_DIR)/test/objs/LINUX -type f -a ! -name *.o` .

clean:
	@$(ECHO) Cleaning $(DVD_BUILD_DIR)
	@$(MAKE) -C $(DVD_BUILD_DIR)/stvid_core clean
	@$(MAKE) -C $(DVD_BUILD_DIR)/stvid_ioctl clean
	@$(MAKE) -C $(DVD_BUILD_DIR)/stvidtest_core clean
	@$(MAKE) -C $(DVD_BUILD_DIR)/stvidtest_ioctl clean
	@$(MAKE) -C $(DVD_BUILD_DIR)/test clean

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End
