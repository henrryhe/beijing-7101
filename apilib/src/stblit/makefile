########################
# MODULE : stblit.lib #
########################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak


#define the targets and exported files
#-------------------------------------
.PHONY: build_all_linux build_linux_tests

COMPONENT_NAME := stblit
ST20_TARGETS := $(COMPONENT_NAME).lib
ST20_EXPORTS := $(COMPONENT_NAME).h $(COMPONENT_NAME).lib

ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))

ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))

LINUX_TARGETS := build_all_linux
ifdef BLIT_FULL_USER
LINUX_EXPORTS_USER := $(call ST20LIB_TO_LINUXLIB,$(ST20_EXPORTS))
LINUX_EXPORTS := $(LINUX_EXPORTS_USER) stblit_ioctl.ko
else
LINUX_EXPORTS := $(call ST20LIB_TO_LINUXLIB,$(ST20_EXPORTS)) stblit_core.ko stblit_ioctl.ko
endif

ifdef BUILD_TESTS
  LINUX_TARGETS += build_linux_tests
  LINUX_TEST_EXPORTS := blt_test
endif


# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# No component in the top-level : run make in ./src
#---------------------------------------------------

FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(OBJECT_DIRECTORY)

$(ST20_TARGETS): $(FULLDIR)/$(ST20_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST20_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(FULLDIR)/$(ST40_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST40_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(FULLDIR)/$(ST200_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST200_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src
endif

ifeq "$(BUILDING_FOR)" "LINUX"
LINUX_EXPORTS_WITHOUT_HEADERS := $(filter-out %.h,$(LINUX_EXPORTS))
ifdef BLIT_FULL_USER
	LINUX_EXPORTS_WITHOUT_HEADERS_USER := $(filter-out %.h,$(LINUX_EXPORTS_USER))
endif

build_all_linux:
	$(ECHO) Building all LINUX targets
ifdef BLIT_FULL_USER
	$(ECHO) Building all LINUX targets
	$(MAKE) -C $(DVD_BUILD_DIR)/linux
	$(MAKE) -C $(DVD_BUILD_DIR)/src
	$(CP) $(addprefix $(DVD_BUILD_DIR)/src/hal/objs/$(OBJECT_DIRECTORY)/,$(LINUX_EXPORTS_WITHOUT_HEADERS_USER)) $(DVD_BUILD_DIR)/linux/objs/$(OBJECT_DIRECTORY)/
	$(CP) $(addprefix $(DVD_BUILD_DIR)/linux/objs/$(OBJECT_DIRECTORY)/,$(LINUX_EXPORTS_WITHOUT_HEADERS)) .
else
	$(MAKE) -C $(DVD_BUILD_DIR)/linux
	$(CP) $(addprefix $(DVD_BUILD_DIR)/linux/objs/$(OBJECT_DIRECTORY)/,$(LINUX_EXPORTS_WITHOUT_HEADERS)) .
endif

build_linux_tests:
	$(ECHO) Building all LINUX tests
	$(MAKE) -C $(DVD_BUILD_DIR)/tests
	$(CP) $(addprefix $(DVD_BUILD_DIR)/tests/objs/$(OBJECT_DIRECTORY)/,$(LINUX_TEST_EXPORTS)) .
endif


# Clean the lib
#--------------
clean: subdir_clean
	-$(RM) $($(OBJECT_DIRECTORY)_TARGETS)
subdir_clean:
ifeq "$(BUILDING_FOR)" "LINUX"
	$(MAKE) -C $(DVD_BUILD_DIR)/linux clean
	$(MAKE) -C $(DVD_BUILD_DIR)/tests/src clean
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean
else
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean
endif

# Default rule
#-------------
FORCE:

else
include $(DVD_MAKE)/builddir.mak
endif


