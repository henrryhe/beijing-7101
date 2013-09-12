######################
# MODULE : video.lib #
######################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak


#define the targets and exported files
#-------------------------------------
ST20_TARGETS := video.lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
LINUX_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ifneq (,$(findstring $(DVD_BACKEND),5528 7020 ))
    DVD_CFLAGS := $(DVD_CFLAGS) -DSTLAYER_VIDEO_TASK
endif

# Pick up standars targets
include $(DVD_MAKE)/defrules.mak

# No component in this level : run make in ./hal
#------------------------------------------------
FULLDIR := $(DVD_BUILD_DIR)/hal/objs/$(OBJECT_DIRECTORY)
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(FULLDIR)/$(ST20_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST20_TARGETS): FORCE
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


ifeq "$(BUILDING_FOR)" "LINUX"
$(LINUX_TARGETS): $(FULLDIR)/$(LINUX_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(LINUX_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/hal
endif

# Clean the lib
#--------------
clean: subdir_clean
	$(RM) $($(BUILDING_FOR)_TARGETS)

subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/hal clean

# Default rule
#-------------
FORCE:

else
include $(DVD_MAKE)/builddir.mak
endif




