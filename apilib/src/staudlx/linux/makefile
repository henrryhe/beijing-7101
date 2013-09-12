# COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

.PHONY: staudlx_tests


LINUX_TARGETS := staudlx_core.ko staudlx_ioctl.ko libstaudlx.a

ifdef STAUDLX_ALSA_SUPPORT
LINUX_TARGETS += staudlx_alsa.ko
endif 

ifdef STAUDLX_DVB_SUPPORT
LINUX_TARGETS += staudlx_dvb.ko
endif 


ifdef BUILD_TESTS
LINUX_TARGETS += staudlx_tests
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

staudlx_tests:
	@$(ECHO) Building all LINUX tests
	@$(MAKE) -C $(DVD_BUILD_DIR)/tests
	@$(CP) `find $(DVD_BUILD_DIR)/tests/objs/LINUX -type f -a ! -name *.o` .

clean:
	@$(ECHO) Cleaning $(DVD_BUILD_DIR)
	@$(MAKE) -C $(DVD_BUILD_DIR)/staudlx_core clean
	@$(MAKE) -C $(DVD_BUILD_DIR)/staudlx_ioctl clean
ifdef STAUDLX_ALSA_SUPPORT	
	@$(MAKE) -C $(DVD_BUILD_DIR)/staudlx_alsa clean
endif
ifdef STAUDLX_DVB_SUPPORT	
	@$(MAKE) -C $(DVD_BUILD_DIR)/staudlx_dvb clean
endif


# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End

