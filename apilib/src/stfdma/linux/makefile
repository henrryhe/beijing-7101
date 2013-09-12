# COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

.PHONY: stfdma_tests

#LINUX_TARGETS := stfdma_core.ko stfdma_ioctl.ko libstfdma.a 
LINUX_TARGETS := stfdma_core.ko

ifdef BUILD_TESTS
#LINUX_TARGETS += stfdma_tests
LINUX_TARGETS += test.ko
endif

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

#stfdma_tests:
#	@$(ECHO) Building all LINUX tests
#	@$(MAKE) -C $(DVD_BUILD_DIR)/test
#	@$(CP) `find $(DVD_BUILD_DIR)/test/objs/LINUX -type f -a ! -name *.o` .

clean:
	@$(ECHO) Cleaning $(DVD_BUILD_DIR)
	@$(MAKE) -C $(DVD_BUILD_DIR)/stfdma_core clean
#	@$(MAKE) -C $(DVD_BUILD_DIR)/stfdma_ioctl clean
	@$(MAKE) -C $(DVD_BUILD_DIR)/test clean

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End
