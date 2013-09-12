
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR

# Pick up default rules and variables
#---------------------
include $(DVD_MAKE)/generic.mak

COMPONENT_NAME := stcommon

ifeq "$(DVD_OS)" "OS40"
 ST40_IMPORTS := os20emu
endif

ifeq "$(DVD_BUILDING_IN_VOB)" "false"
  ifeq "$(DVD_OS)" "OS40"
    HEADER_IMPORTS += os20emu
  endif
endif

ST20_IMPORTS = stos
ST40_IMPORTS += stos
ST200_IMPORTS = stos

ifeq "$(DVD_OS)" "WINCE"
HEADER_IMPORTS := 
ST20_IMPORTS := 
ST40_IMPORTS := $(ST20_IMPORTS)
endif

# Define the targets
#-------------------

ST20_TARGETS := $(COMPONENT_NAME).lib
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))

ST20_EXPORTS := stcommon.h stcommon.lib
ST40_EXPORTS := stcommon.h libstcommon.a
ST200_EXPORTS := stcommon.h $(LIB_PREFIX)stcommon$(LIB_SUFFIX)


# Local objects which comprise this component
ST20_OBJS := stcommon.tco stclock.tco
ST40_OBJS := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST200_OBJS := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))


ifeq "$(DVD_OS)" "WINCE"
# Add include path
#---------------------
INCLUDE_PATH +=  $(addprefix $(DVD_BUILD_DIR)/,$(SUBDIRS))

# Compile options
#----------------
CFLAGS := $(COMMON_DEFS) $(CFLAGS)
endif

# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
ifeq "$(BUILDING_FOR)" "ST20"
$(ST20_TARGETS): $(ST20_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
$(ST40_TARGETS): $(ST40_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST200"
$(ST200_TARGETS): $(ST200_OBJS)
	$(BUILD_LIBRARY)
endif

ifeq "$(BUILDING_FOR)" "ST40"
ifeq "$(DVD_OS)" "WINCE"
$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)
endif
endif

# Clean .lib and obj
#-------------------
clean:
	-$(RM) $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)


ifeq "$(DVD_OS)" "WINCE"
clean:$(SUBDIR_CLEAN)
	-$(RM) $($(BUILDING_FOR)_TARGETS)
	-$(RM) $($(BUILDING_FOR)_OBJS)
endif

# Local header dependencies

else

include $(DVD_MAKE)/builddir.mak

endif

# End
