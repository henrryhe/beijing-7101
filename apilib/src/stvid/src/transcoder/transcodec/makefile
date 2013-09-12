##########################
# MODULE : transcodec.lib  #
##########################

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
IMPORTS := sttbx stevt stavmem stlayer stgxobj stos
ifneq ($(findstring inject,$(VID_OPTIONAL_MODULES)),)
IMPORTS := $(IMPORTS) stfdma
endif
ifneq ($(findstring avsync,$(VID_OPTIONAL_MODULES)),)
IMPORTS := $(IMPORTS) stclkrv
endif
ifdef VALID_TOOLS
IMPORTS := $(IMPORTS) valitls
endif

ST20_TARGETS := transcodec.lib
ST20_OBJS := transcodec.tco

ST40_IMPORTS := $(IMPORTS)
ST40_EXPORTS := $(call ST20LIB_TO_ST40LIB,$(ST20_EXPORTS))
ST40_TARGETS := $(call ST20LIB_TO_ST40LIB,$(ST20_TARGETS))
ST40_OBJS    := $(call ST20OBJ_TO_ST40OBJ,$(ST20_OBJS))

# Sub-libs for vidcodec :
#------------------------
ifneq ($(findstring mpg2toh264,$(VID_OPTIONAL_TRANSCODECS)),)
SUBDIRS := $(SUBDIRS) mpg2toh264
HEADER_DEPS := $(HEADER_DEPS) mpg2toh264/mpg2toh264.h
endif

# General rules
CFLAGS := $(VID_DEFS) $(CFLAGS)

# Add definitions for codecs
CFLAGS += $(addprefix -D ST_,$(VID_OPTIONAL_TRANSCODECS))

# Add include paths
INCLUDE_PATH += $(DVD_BUILD_DIR)/../.. $(DVD_BUILD_DIR)/.. $(DVD_BUILD_DIR)/../../..
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)/../../,$(VID_OTHER_MODULES_DEPENDENCIES))
INCLUDE_PATH += $(addprefix $(DVD_BUILD_DIR)/,$(VID_OPTIONAL_TRANSCODECS))

# Pass definitions to sub make calls
export VID_OTHER_MODULES_DEPENDENCIES
export VID_OPTIONAL_TRANSCODECS

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

clean_message: FORCE
	@$(ECHO) Cleaning transcodec

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
transcodec.tco transcodec.o : ../transcoder.h transcodec.h $(HEADER_DEPS)

#end of file
