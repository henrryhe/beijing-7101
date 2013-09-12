DVD_MAKE_VERSION := 2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

HEADER_IMPORTS := stmerge $(DVD_TRANSPORT) stinj

# Define the targets and exported files
#--------------------------------------
COMPONENT_NAME := stnet
ST20_TARGETS := $(COMPONENT_NAME).lib
ST20_EXPORTS := $(COMPONENT_NAME).h $(ST20_TARGETS)

ifneq "$(DVD_OS)" "LINUX"
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
else
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST40_EXPORTS := $(call ST20LIB_TO_LINUXLIB,$(ST20_EXPORTS))
endif

FULLDIR := $(DVD_BUILD_DIR)/src/objs/$(OBJECT_DIRECTORY)

$($(ARCHITECTURE)_TARGETS): $(FULLDIR)/$($(ARCHITECTURE)_TARGETS)
	$(CP) $(subst $(BAD_SLASH),$(GOOD_SLASH),$<) $@ > $(NULL)


$(FULLDIR)/$($(ARCHITECTURE)_TARGETS): FORCE
	@$(ECHO) ---- Entering src directory ----
	$(MAKE) -C $(DVD_BUILD_DIR)/src

clean: subdir_clean
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	$(RM) $($(ARCHITECTURE)_TARGETS)

subdir_clean:
	$(MAKE) -C $(DVD_BUILD_DIR)/src clean

FORCE:

else

include $(DVD_MAKE)/builddir.mak

endif

# end
