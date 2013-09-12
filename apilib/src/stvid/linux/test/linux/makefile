# COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

.PHONY: stvidtest_tests

LINUX_TARGETS := stvidtest_core.ko stvidtest_ioctl.ko libstvidtest.a 

ifdef STVID_BUILD_TESTS
LINUX_TARGETS += stvidtest_tests
endif

include $(DVD_MAKE)/defrules.mak

# For building kernel objects
$(filter %.ko,$(LINUX_TARGETS)): FORCE
	@$(ECHO) Building $@
	@$(MAKE) -C $(DVD_BUILD_DIR)/$(basename $@)
	@$(CP) $(DVD_BUILD_DIR)/$(basename $@)/$@ .

# For building libraries
$(filter lib%.a,$(LINUX_TARGETS)): FORCE
	@$(ECHO) Building $@
	@$(MAKE) -C $(DVD_BUILD_DIR)/$(patsubst lib%.a,%_ioctl,$@) $@
	@$(CP) $(DVD_BUILD_DIR)/$(patsubst lib%.a,%_ioctl,$@)/$@ .

FORCE:

stvidtest_tests:
	@$(ECHO) Building all LINUX tests
	@$(MAKE) -C $(DVD_BUILD_DIR)/test
	@$(CP) `find $(DVD_BUILD_DIR)/test/objs/LINUX -type f -a ! -name *.o` .

clean:
	@$(ECHO) Cleaning $(DVD_BUILD_DIR)
	@$(MAKE) -C $(DVD_BUILD_DIR)/stvidtest_core clean
	@$(MAKE) -C $(DVD_BUILD_DIR)/stvidtest_ioctl clean
	@$(MAKE) -C $(DVD_BUILD_DIR)/test clean

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End
