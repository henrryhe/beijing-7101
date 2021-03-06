############################
# MODULE : parser.lib  #
############################

VID_OTHER_MODULES_DEPENDENCIES =
ifneq ($(findstring inject,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) inject
endif
ifneq ($(findstring fgt,$(VID_OPTIONAL_MODULES)),)
    VID_OTHER_MODULES_DEPENDENCIES := $(VID_OTHER_MODULES_DEPENDENCIES) fgt
endif

# Add for make system version 2 support
DVD_MAKE_VERSION := 2
ifdef IN_OBJECT_DIR
# Pick up default variables
include $(DVD_MAKE)/generic.mak

# Local objects which comprise this component
#---------------------------------------------
ST20_OBJS := mpg2parser.tco	mpg2parsertsk.tco mpg2scparse.tco mpg2getbits.tco mpg2dtvud.tco
ST20_TARGETS := parser.lib
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
INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../fgt
#INCLUDE_PATH += $(DVD_BUILD_DIR)/../../ $(DVD_BUILD_DIR)/../ $(DVD_BUILD_DIR)/../../../ $(DVD_BUILD_DIR)/../../../../
#INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../decode $(DVD_BUILD_DIR)/../../../../decode/hal
#INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../buffers $(DVD_BUILD_DIR)/../../../../ordqueue
#INCLUDE_PATH += $(DVD_BUILD_DIR)/../../../../inject

#Pick up standard targets
include $(DVD_MAKE)/defrules.mak

# Build the lib
#---------------
$(ST20_TARGETS): $(ST20_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

$(ST40_TARGETS): $(ST40_OBJS) $(SUBDIR_LIBS)
	$(BUILD_LIBRARY)

clean_message: FORCE
	@$(ECHO) Cleaning MPG2 Video CODEC parser $(ST20_TARGETS)

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
mpg2parser.tco mpg2parser.tco  : ./mpg2parser.h ../vcodmpg2.h
mpg2parsertsk.tco mpg2parsertsk.o : ./mpg2parser.h ../vcodmpg2.h
mpg2scparse.tco  mpg2scparse.o : ./mpg2parser.h ../vcodmpg2.h
mpg2getbits.tco mpg2getbits.o : ./mpg2parser.h ../vcodmpg2.h
mpg2dtvud.tco mpg2dtvud.o : ./mpg2parser.h ../vcodmpg2.h

#end of file
