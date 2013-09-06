# vi:noexpandtab
######################
# MODULE : stbdf.lib #
######################


DVD_MAKE_VERSION :=2

ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

HEADER_IMPORT += stblit stgxobj

SUB_COMPONENT := blit
    
ST20_TARGETS := $(SUB_COMPONENT).lib
ST20_OBJS    := stblitwrap_cmd.tco

PATH_SRC_DIR := $(dir $(DVD_BUILD_DIR))
   
INCLUDE_PATH := $(PATH_SRC_DIR)/gfx $(INCLUDE_PATH)

# Pick up standard targets
include $(DVD_MAKE)/defrules.mak

$(ST20_TARGETS): $(ST20_OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean:
	@echo Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

# Local Dependancies


else

include $(DVD_MAKE)/builddir.mak

endif
