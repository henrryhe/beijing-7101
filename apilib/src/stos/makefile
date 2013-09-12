#################################
# MODULE : stos.lib / libstos.a #
#################################
# Add for make system version 2 support
DVD_MAKE_VERSION := 2

# If DEBUG_VERSION is defined in environment, assert() calls are executed
ifndef DEBUG
    DVD_CFLAGS += -DNDEBUG
endif

ifdef IN_OBJECT_DIR
# Pick up default rules and variables
include $(DVD_MAKE)/generic.mak
#--------------------------
# ADD OS21 Support
ifeq "$(DVD_OS)" "OS21"
SUBDIRS     := os21
endif

#--------------------------
# ADD OS20 Support
ifeq "$(DVD_OS)" "OS20"
SUBDIRS     := os20
endif

#--------------------------
# ADD Emulation OS21 for Wince
ifeq "$(DVD_TOOLSET)" "ST40WINCE"
SUBDIRS     := wince_os
endif

ifeq "$(DVD_OS)" "LINUX"
# Under Linux, dependency with stcommon
HEADER_IMPORTS := stcommon
endif


# Define the targets
#-------------------
COMPONENT_NAME := stos
IMPORTS :=
ST20_EXPORTS := stos.h stos.lib
ST20_TARGETS := stos.lib
ST20_OBJS := stos_rev.tco

ST40_IMPORTS := $(IMPORTS)
ifeq "$(DVD_OS)" "LINUX"
ST40_EXPORTS := $(call ST20LIB_TO_LINUXLIB,$(ST20_EXPORTS))
ST40_TARGETS := $(call ST20LIB_TO_LINUXLIB,$(ST20_TARGETS))
ST40_OBJS    := $(call ST20OBJ_TO_LINUXOBJ,$(ST20_OBJS))
else
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
endif

ST200_IMPORTS := $(IMPORTS)
ST200_EXPORTS := $(call ST20LIB_TO_ST200LIB,$(ST20_EXPORTS))
ST200_TARGETS := $(call ST20LIB_TO_ST200LIB,$(ST20_TARGETS))
ST200_OBJS    := $(call ST20OBJ_TO_ST200OBJ,$(ST20_OBJS))


# Pick up standard rules
#-----------------------
include $(DVD_MAKE)/defrules.mak

# Build lib and obj
#------------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	@$(ECHO) Building $@
	$(BUILD_LIBRARY)

# Clean .lib and obj
#-------------------
clean: $(SUBDIR_CLEAN)
	@$(ECHO) Cleaning $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)
	-$(RM) $($(ARCHITECTURE)_OBJS)

else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependencies
stos.tco stos.o: stos.h

