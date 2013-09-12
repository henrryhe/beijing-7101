##################
# MODULE : stprm #
##################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

COMPONENT_NAME := stprm

ifeq "$(DVD_BUILDING_IN_VOB)" "true"
 COMPONENT_EXT := rel
endif

ST20_TARGETS := $(COMPONENT_NAME)$(COMPONENT_EXT).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST20_EXPORTS := stprm.h $(COMPONENT_NAME)$(COMPONENT_EXT).lib
ST40_EXPORTS := stprm.h $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST200_EXPORTS := stprm.h $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))

LIB_OPTION :=
ifdef STPRM_PTI_RECORD_BUFFER_SUPPORT
 LIB_OPTION := $(LIB_OPTION)_recbuf
endif
#debug lib
ifdef STPRM_DEBUG
 LIB_OPTION := $(LIB_OPTION)_d
endif

include $(DVD_MAKE)/defrules.mak

$(LIB_PREFIX)$(COMPONENT_NAME)$(COMPONENT_EXT)$(LIB_SUFFIX) : force
	$(CP) ..$(SLASH)..$(SLASH)$(COMPONENT_NAME)_$(DVD_FRONTEND)_$(DVD_OS_LC)$(LIB_OPTION)_lib $@ > $(NULL)
ifeq "$(DVD_EXPORT_LIBS)" "true"
	$(RM) $(subst $(BAD_SLASH),$(GOOD_SLASH),$(DVD_$(ARCHITECTURE)_EXPORTS)$(SLASH)$@)
endif

#force to build
force:

clean:
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	@$(RM) $($(ARCHITECTURE)_TARGETS)

else

include $(DVD_MAKE)/builddir.mak

endif

# End
