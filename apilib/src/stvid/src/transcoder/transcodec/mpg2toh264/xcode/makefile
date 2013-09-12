#######################
# MODULE : xcode.lib  #
#######################

# video specific variables
VID_OTHER_MODULES_DEPENDENCIES =
ifneq ($(findstring inject,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) inject
endif

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default rvariables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := mpg2toh264xcode.tco
ifdef STVID_VALID
ST20_OBJS += mpg2toh264xcode_dump.tco
endif

ST20_TARGETS := xcode.lib
ST20_IMPORTS := sttbx stevt stfdma stavmem stos
ifdef VALID_TOOLS
ST20_IMPORTS := $(ST20_IMPORTS) valitls
endif

ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_EXPORTS := $(ST20_EXPORTS)
ST40_IMPORTS := $(ST20_IMPORTS)

# General rules
CFLAGS := $(VID_DEFS) $(CFLAGS)

CFLAGS += $(addprefix -D ST_,$(VID_OPTIONAL_MODULES))

# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../../.. $(DVD_BUILD_DIR)/../../../..
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../inject
INCLUDE_PATH += $(RPC_ROOT)/include
INCLUDE_PATH += $(RPC_ROOT)/src/mme/include
INCLUDE_PATH += $(RPC_ROOT)/src/embx/include

#Pick up standard xcodes
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

clean_message: FORCE
	@$(ECHO) Cleaning Transcodec MPEG2 to H264 xcode decoder $(ST20_TARGETS)

clean: clean_message $(SUBDIR_CLEAN)
	-$(RM) $($(ARCHITECTURE)_OBJS)
	-$(RM) $($(ARCHITECTURE)_TARGETS)

FORCE:


# Add for make system version 2 support
#---------------------------------------
else
include $(DVD_MAKE)/builddir.mak
endif

# Local dependancies
mpg2toh264xcode.tco mpg2toh264xcode.o : ./mpg2toh264xcode.h  ./mpg2toh264xcode_dump.h
ifdef STVID_VALID
mpg2toh264xcode_dump.tco mpg2toh264xcode_dump.o : ./mpg2toh264xcode.h ./mpg2toh264xcode_dump.h
endif
#end of file
