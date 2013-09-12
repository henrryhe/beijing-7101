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
COMPONENT_NAME := stblit
ST20_TARGETS := $(COMPONENT_NAME).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))




# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# No component in the top-level : run make in ./src
#---------------------------------------------------
ifeq "$(DVD_OS)" "LINUX"

ifdef BLIT_FULL_USER

FULLDIR := $(DVD_BUILD_DIR)/hal/objs/$(OBJECT_DIRECTORY)

else #BLIT_FULL_USER

FULLDIR := $(DVD_BUILD_DIR)/hal/objs/$(OBJECT_DIRECTORY)

endif

else  #LINUX

FULLDIR := $(DVD_BUILD_DIR)/hal/objs/$(OBJECT_DIRECTORY)

endif

$(ST20_TARGETS): $(FULLDIR)/$(ST20_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST20_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/hal


ifeq "$(BUILDING_FOR)" "LINUX"

$(LINUX_TARGETS): $(FULLDIR)/$(LINUX_TARGETS)
	$(BUILD_LIBRARY)

ifdef BLIT_FULL_USER
	$(CP) $(FULLDIR)/$(ST40_TARGETS) $(DVD_BUILD_DIR)/../objs/$(OBJECT_DIRECTORY)
else
	$(AR) -x $(FULLDIR)/liblinux.a
	$(AR) -rsc $(ST40_TARGETS) ustblit.o
endif

$(FULLDIR)/$(LINUX_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/hal


endif


ifeq "$(BUILDING_FOR)" "ST40"

$(ST40_TARGETS): $(FULLDIR)/$(ST40_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST40_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/hal

endif


ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(FULLDIR)/$(ST200_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST200_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/hal
endif




# Clean the lib
#--------------
clean: subdir_clean
	-$(RM) $($(OBJECT_DIRECTORY)_TARGETS)
subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/hal clean

# Default rule
#-------------
FORCE:

else
include $(DVD_MAKE)/builddir.mak
endif
