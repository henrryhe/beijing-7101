DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
IMPORTS := stos
ifeq "$(DVD_OS)" "OS40"
IMPORTS += os20emu
endif
EXPORTS := stpio.h $(LIB_PREFIX)stpio$(LIB_SUFFIX)
TARGETS := $(LIB_PREFIX)stpio$(LIB_SUFFIX)

# ST40
ifndef ST40_TARGETS
ST40_TARGETS = $(TARGETS)
endif

ifndef ST40_EXPORTS
ST40_EXPORTS = $(EXPORTS)
endif

ifndef ST40_IMPORTS
ST40_IMPORTS = $(IMPORTS)
endif

# ST200
ifndef ST200_TARGETS
ST200_TARGETS = $(TARGETS)
endif

ifndef ST200_EXPORTS
ST200_EXPORTS = $(EXPORTS)
endif

ifndef ST200_IMPORTS
ST200_IMPORTS = $(IMPORTS)
endif

# local objects which comprise this component
OBJS := stpio$(OBJ_SUFFIX)

# if using os40, add os20emu to header imports
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

ifdef STAPI_INTERRUPT_BY_NUMBER
    CFLAGS := $(CFLAGS) -DSTAPI_INTERRUPT_BY_NUMBER
endif

include $(DVD_MAKE)/defrules.mak

$(LIB_PREFIX)stpio$(LIB_SUFFIX): $(OBJS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

clean:
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $(OBJS)

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End
