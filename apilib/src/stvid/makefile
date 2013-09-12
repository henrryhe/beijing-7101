######################
# MODULE : stvid.lib #
######################

# Add for make system version 2 support
DVD_MAKE_VERSION :=2

ifdef IN_OBJECT_DIR

# Pick up default variables
include $(DVD_MAKE)/generic.mak

#define the targets and exported files
#-------------------------------------
COMPONENT_NAME := stvid

ST20_TARGETS := $(COMPONENT_NAME).lib
ST20_EXPORTS := $(COMPONENT_NAME).h $(COMPONENT_NAME).lib

ifeq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_LINUXLIB,$(ST20_EXPORTS))
else
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
endif

ST200_TARGETS := $(ST40_TARGETS)
ST200_EXPORTS := $(ST40_EXPORTS)

# Other components this component is dependant upon
IMPORTS := stevt stavmem stpio sttbx stvtg stclkrv stlayer stgxobj stos
ifeq "$(DVD_OS)" "LINUX"
IMPORTS += stcommon
else
IMPORTS += stuart
endif


ST40_IMPORTS := $(IMPORTS)

# Pick up standars rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# No component in the top-level : run make in ./src
#---------------------------------------------------
ifeq "$(DVD_OS)" "LINUX"
FULLDIR := $(DVD_BUILD_DIR)/linux/stvid_ioctl
else
FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(OBJECT_DIRECTORY)
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
ifeq "$(DVD_OS)" "LINUX"
$(FULLDIR)/$(ST40_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/linux/stvid_ioctl
else
$(FULLDIR)/$(ST40_TARGETS): FORCE
	$(MAKE) -C $(DVD_BUILD_DIR)/src
endif
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

#end of file
