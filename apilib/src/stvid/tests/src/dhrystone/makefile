#######################################################################
# Makefile for dhrystone
#######################################################################

DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

include $(DVD_MAKE)/generic.mak

# Other components this component is dependant upon
HEADER_IMPORTS := stsys stos
ifeq "$(DVD_BUILDING_IN_VOB)" "false"
    ifeq "$(DVD_OS)" "OS40"
        HEADER_IMPORTS += os20emu
    endif
endif

COMMON_IMPORTS := stcommon
ST20_IMPORTS :=  $(COMMON_IMPORTS)
ST40_IMPORTS :=  $(COMMON_IMPORTS) os20emu
ST200_IMPORTS := $(COMMON_IMPORTS)

# Object
COMMON_OBJS := main.tco dhry21a.tco dhry21b.tco timers_b.tco
ST20_OBJS := $(COMMON_OBJS)
# Object for ST40
ST40_OBJS = $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Object for ST200
ST200_OBJS = $(ST40_OBJS)

# USE_REG to use "register" C keyword
CFLAGS := $(CFLAGS) -DUSE_REG

# Target
ST20_TARGETS  = dhrystone.lib
ST40_TARGETS  = $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS = $(ST40_TARGETS)

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build library
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)

# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

else

include $(DVD_MAKE)/builddir.mak

endif
