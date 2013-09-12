#####################################
# MODULE : stvtg.lib / libstvtg.a #
#####################################
# Add for make system version 2 support
DVD_MAKE_VERSION :=2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

#define the targets and exported files
#-------------------------------------

COMPONENT_NAME := stvtg
ST20_TARGETS := $(COMPONENT_NAME).lib
ST20_EXPORTS := $(COMPONENT_NAME).h $(COMPONENT_NAME).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))


# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# No component in the top-level : run make in ./src
#---------------------------------------------------
FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(OBJECT_DIRECTORY)

ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(FULLDIR)/$(ST20_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$< $@ > $(NULL))
$(FULLDIR)/$(ST20_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src
endif

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


# Clean the lib
#--------------
clean: subdir_clean
	-$(RM) $($(BUILDING_FOR)_TARGETS)
subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean


# Default rule
#-------------
FORCE:

else
include $(DVD_MAKE)/builddir.mak
endif

