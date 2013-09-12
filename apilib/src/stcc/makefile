#####################################
# MODULE : stcc.lib / libstcc.a #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

#define the targets and exported files
#-------------------------------------
COMPONENT_NAME := stcc
ST20_TARGETS := $(COMPONENT_NAME).lib
ST20_EXPORTS := $(COMPONENT_NAME).h $(COMPONENT_NAME).lib

ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))


ifeq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_LINUXLIB,$(ST20_EXPORTS))
else
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
endif

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# No component in the top-level : run make in ./src
#---------------------------------------------------
ifdef USER_WRAPPER
FULLDIR := $(DVD_BUILD_DIR)/linux/stcc_ioctl
else
FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(ARCHITECTURE)
endif

ifeq "$(ARCHITECTURE)" "ST20"
$(ST20_TARGETS): $(FULLDIR)/$(ST20_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST20_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src
endif

ifeq "$(ARCHITECTURE)" "ST40"
$(ST40_TARGETS): $(FULLDIR)/$(ST40_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
ifdef USER_WRAPPER
$(FULLDIR)/$(ST40_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/linux/stcc_ioctl
else
$(FULLDIR)/$(ST40_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src
endif
endif

ifeq "$(ARCHITECTURE)" "ST200"
$(ST200_TARGETS): $(FULLDIR)/$(ST200_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST200_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src
endif

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

