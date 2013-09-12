###########################################
# MODULE : stapigat.lib / libstapigat.a   #
###########################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak


#define the targets and exported files
#-------------------------------------
COMPONENT_NAME := stapigat
ST20_TARGETS := $(COMPONENT_NAME).lib
ST20_EXPORTS := $(COMPONENT_NAME).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
LINUX_EXPORTS := $(call ST20LIB_TO_LINUXLIB,$(ST20_EXPORTS))

# stapigat test kernel object is copied to DVD_EXPORTS/share/lib/modules/
LINUX_TEST_MODULE_EXPORTS := stapigat.ko

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# No component in the top-level : run make in ./src
#---------------------------------------------------
FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(OBJECT_DIRECTORY)
LIBRARY_NAME := $(LIB_PREFIX)$(COMPONENT_NAME)$(LIB_SUFFIX)

$(LIBRARY_NAME): $(FULLDIR)/$(LIBRARY_NAME)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
  ifeq "$(BUILDING_FOR)" "LINUX"
	$(CP) $(FULLDIR)/stapigat.ko . > $(NULL)
  endif

$(FULLDIR)/$(LIBRARY_NAME): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src


# Clean the lib
#--------------
clean: subdir_clean
	-$(RM) $($(ARCHITECTURE)_TARGETS)

subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean

# Default rule
#-------------
FORCE:

else
include $(DVD_MAKE)/builddir.mak
endif

